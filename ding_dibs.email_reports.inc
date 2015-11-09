<?php
/**
 * @file
 * E-mail reports functionality.
 */

/**
 * Settings form.
 */
function ding_dibs_setting_emails($form, &$form_state) {
  drupal_set_title(t('Ding DIBS setting emails'));
  drupal_add_css(drupal_get_path('module', 'ding_dibs') . '/css/ding_dibs.css');
  $options = drupal_map_assoc(range(0, 23));
  $form = array();
  $form['ding_dibs_start_cron'] = array(
    '#type' => 'select',
    '#title' => t('Set the daily hour to run this cron'),
    '#description' => t('Set the daily hour to run this cron.'),
    '#options' => $options,
    '#default_value' => variable_get('ding_dibs_start_cron', 2),
    '#required' => TRUE,
  );
  $form['ding_dibs_emails'] = array(
    '#type' => 'textarea',
    '#title' => t('Input emails that should be notified if something unusual happens with a payment.'),
    '#description' => t('Enter up to ten email addresses Enter through the separator or comma, without blank lines.'),
    '#default_value' => variable_get('ding_dibs_emails', ''),
    '#required' => TRUE,
    '#element_validate' => array('ding_dibs_emails_validate'),
  );
  return system_settings_form($form);
}

/**
 * Email validation function.
 */
function ding_dibs_emails_validate($element, &$form_state, $form) {
  $arrays_email = preg_split("/\,|\r\n|\n/", $element['#value']);
  if (count($arrays_email) < 10) {
    foreach ($arrays_email as $email) {
      $email = trim($email);
      if (!empty($email) && !valid_email_address($email)) {
        form_error($element, t('Please enter a valid email address.'));
      }
      elseif (empty($email)) {
        form_error($element, t('Remove the empty string.'));
      }
    }
  }
  else {
    form_error($element, t('Maximum 10 email addresses to send.'));
  }
}

/**
 * Output report table.
 */
function ding_dibs_email_reports() {
  $time = REQUEST_TIME - 10800;
  $report = db_select('dibs_transactions', 'd')
    ->fields('d', array(
      'payment_order_id',
      'payment_transaction_id',
      'payment_time',
      'customer_uid',
      'order_info_short',
      'order_info_long',
      'payment_price',
      'payment_status',
      'provider_payment_status',
      'dibs_capture_status',
    ))
    ->condition('payment_time', date('Y-m-d H:i:s', $time), '>')
    ->condition('d.api_module', 'ding_dibs')
    ->condition('d.api_delta', 1)
    ->orderBy('payment_order_id', 'DESC')
    ->execute()
    ->fetchAll();
  $max_count = count($report);
  $no_transactions_msg = '<p>' . t('No transactions found.') . '</p>';
  if (!$max_count) {
    return $no_transactions_msg;
  }
  for ($count = 0; $count < $max_count; $count++) {
    if (!($report[$count]->payment_status == $report[$count]->provider_payment_status && $report[$count]->payment_status == $report[$count]->dibs_capture_status && $report[$count]->provider_payment_status == $report[$count]->dibs_capture_status)) {
      $report_valid[] = $report[$count];
    }
  }
  $rows = array();
  drupal_add_css(drupal_get_path('module', 'ding_dibs') . '/css/ding_dibs.css');
  $header = array(
    array(
      'data' => t('Order ID'),
      'class' => 'payment',
    ),
    array(
      'data' => t('Transaction ID'),
      'class' => 'payment',
    ),
    array(
      'data' => t('Date'),
      'class' => 'payment',
    ),
    array(
      'data' => t('User'),
      'class' => 'payment',
    ),
    array(
      'data' => t('Text'),
      'class' => 'payment',
    ),
    array(
      'data' => t('Amount'),
      'class' => 'payment',
    ),
    array(
      'data' => t('Cleared'),
      'class' => 'payment',
    ),
    array(
      'data' => t('Paid'),
      'class' => 'payment',
    ),
    array(
      'data' => t('Captured'),
      'class' => 'payment',
    ),
    array(
      'data' => '',
      'class' => 'payment',
    ),
  );
  foreach ($report_valid as $record) {
    $status = $record->payment_status * 1 . $record->provider_payment_status * 1 . $record->dibs_capture_status * 1;
    switch ($status) {
      case '111':
      case '000':
        $status = 'ok';
        break;

      case '100':
        $status = 'warning';
        break;

      default:
        $status = 'error';
    }
    $order_info_long = unserialize($record->order_info_long);
    if (is_array($order_info_long) && count($order_info_long) > 0) {
      $text = implode(array_shift($order_info_long)) . ' ';
      $text .= implode(', ', array_map('array_shift', $order_info_long));
    }
    elseif (is_string($order_info_long)) {
      $text = $order_info_long;
    }
    else {
      $text = $record->order_info_short;
    }
    $row = array(
      array(
        'data' => check_plain($record->payment_order_id),
        'class' => 'payment',
      ),
      array(
        'data' => check_plain($record->payment_transaction_id),
        'class' => 'payment',
      ),
      array(
        'data' => check_plain($record->payment_time),
        'class' => 'payment',
      ),
      array(
        'data' => check_plain($record->customer_uid),
        'class' => 'payment',
      ),
      array(
        'data' => check_plain($text),
        'class' => 'payment',
      ),
      array(
        'data' => check_plain($record->payment_price),
        'class' => 'payment',
      ),
      array(
        'data' => $record->payment_status ? t('yes') : t('no'),
        'class' => 'payment',
      ),
      array(
        'data' => $record->provider_payment_status ? t('yes') : t('no'),
        'class' => 'payment',
      ),
      array(
        'data' => $record->dibs_capture_status ? t('yes') : t('no'),
        'class' => 'payment',
      ),
      array(
        'data' => '',
        'class' => 'icon ' . $status,
      ),
    );
    $rows[] = $row;
    $no_transactions_msg = FALSE;
  }

  $output = theme('table', array(
    'header' => $header,
    'rows' => $rows,
  ));
  if ($no_transactions_msg) {
    $output .= $no_transactions_msg;
  }
  return render($output);
}

/**
 * Output report cron status.
 */
function ding_dibs_cron_report() {
  drupal_set_title(t('Ding DIBS cron report'));
  $report = db_select('ding_dibs_report_cron', 'd')
    ->fields('d', array('id', 'time_start', 'status', 'text'))
    ->orderBy('id', 'DESC')
    ->execute()
    ->fetchAll();
  $max_count = count($report);
  $no_transactions_msg = '<p>' . t('No cron report.') . '</p>';
  if (!$max_count) {
    return $no_transactions_msg;
  }
  $rows = array();
  drupal_add_css(drupal_get_path('module', 'ding_dibs') . '/css/ding_dibs.css');
  $header = array(array('data' => t('Message'), 'class' => 'payment'));
  foreach ($report as $record) {
    if (empty($record->text)) {
      $day = format_date($record->time_start, 'custom', 'd');
      $hour = format_date($record->time_start, 'custom', 'H:i');
      $record->text = $day . 'th september ' . $hour . ': Job failed.';
    }
    $row = array(
      array(
        'data' => t($record->text),
        'class' => 'payment',
      ),
    );
    $rows[] = $row;
    $no_transactions_msg = NULL;
  }
  $output = theme('table', array(
    'header' => $header,
    'rows' => $rows,
  ));
  if ($no_transactions_msg) {
    $output .= $no_transactions_msg;
  }
  return render($output);
}
