<?php

/**
 *
 */
function commerce_tpay_verify()
{

  if (!empty($_POST)) {
    $order_id = explode(':', $_POST['tr_crc']);
    $order = commerce_order_load($order_id[0]);

    $payment_method_instance_id = $order->data['payment_method'];
    $payment_method = commerce_payment_method_instance_load($payment_method_instance_id);


    $amount = $_POST['tr_amount'];
    $crc = $_POST['tr_crc'];
    $tr_id = $_POST['tr_id'];
    $sid = $payment_method['settings']['sid'];
    $pmid = $payment_method['settings']['cod'];
    $md5 = $sid . $tr_id . $amount . $crc . $pmid;

    $tr_id = $_POST['tr_id'];

    $transaction_status = $_POST['tr_status'];

    if ($sid != $payment_method['settings']['sid']) {
      $watchdog_variables['@pos_id'] = $sid;
      watchdog('commerce_tpay', 'ERROR bad pos_id: @pos_id.', $watchdog_variables, WATCHDOG_WARNING);
      print t('ERROR BAD pos_id: @pos_id.', $watchdog_variables);
      exit();
    }


    if (in_array($_SERVER['REMOTE_ADDR'], explode(',', $payment_method['settings']['ip']))) {

      if (md5($md5) == $_POST['md5sum']) {

        if (empty($transaction)) {
          $transaction = commerce_payment_transaction_new('tpay', $order->order_id);
          $transaction->created = time();
        }
        $transaction->instance_id = $payment_method['instance_id'];
        $transaction->payment_method = 'tpay';
        $transaction->remote_id = $tr_id;
        $transaction->currency_code = 'PLN';
        $transaction->amount = $amount*100;
        $transaction->payload[REQUEST_TIME] = $_POST;

        $transaction->changed = time();

        $transaction->message = t('Payment status: @status', array('@status' => isset($transaction_status) ? t('Paid') : $transaction_status));
        if ($order->status != 'pending') {
          rules_invoke_all('commerce_checkout_complete', $order);
        }

        switch ($_POST['tr_error']) {
          case 'overpay':
          case 'surcharge':
            $transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
            break;
          case 'none':
            if ($transaction_status == 'TRUE') {
              $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
              $order->status = 'processing';
            }
            break;
        }
        $transaction->remote_status = $transaction->status;
        commerce_payment_transaction_save($transaction);
        commerce_order_save($order);
        echo "TRUE";
        return;
      }
      else{
        watchdog('commerce_tpay', 'Invalid checksum: @md5', array('@md5' => $md5), WATCHDOG_WARNING);
      }
    }
    else{
        watchdog('commerce_transferuj_pl', 'Invalid Remote addr: @addr', array('@addr' => $_SERVER['REMOTE_ADDR']), WATCHDOG_WARNING);
    }
  }
  echo "FALSE";
}
