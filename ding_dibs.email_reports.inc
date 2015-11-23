<?php
/**
 * @file
 * E-mail reports functionality.
 */


/**
 * Form settings mailing reports.
 *
 * @return
 *   Save results form settings.
 */
function ding_dibs_setting_emails($form, &$form_state) {
  drupal_add_css(drupal_get_path('module', 'ding_dibs') . '/css/ding_dibs.css');
  $hour = range(0, 23);
  $options_hour = array_map(_ding_dibs_generate_zero, $hour);
  $minute = range(0, 59);
  $options_minute = array_map(_ding_dibs_generate_zero, $minute);
  $form = array();
  $form['ding_dibs'] = array(
    '#type' => 'fieldset',
    '#title' => t('Set the daily hour to run this cron'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '<div class="description">' . t('Set the daily hh:ii to run this cron.') . '</div></div>',
  );
  $form['ding_dibs']['ding_dibs_start_cron_hour'] = array(
    '#type' => 'select',
    '#options' => $options_hour,
    '#default_value' => variable_get('ding_dibs_start_cron_hour', 2),
    '#required' => TRUE,
  );
  $form['ding_dibs']['ding_dibs_start_cron_minute'] = array(
    '#type' => 'select',
    '#options' => $options_minute,
    '#default_value' => variable_get('ding_dibs_start_cron_minute', 0),
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
 * Validation forms element email report settings.
 */
function ding_dibs_emails_validate($element, &$form_state, $form) {
  $arrays_email = preg_split("/\,|\r\n|\n/", $element['#value']);
  if (!empty($arrays_email)) {
    foreach ($arrays_email as $email) {
      $email = trim($email);
      if (!empty($email) && !valid_email_address($email)) {
        form_error($element, t('Please enter a valid email address.'));
      }
    }
  }
}

/**
 * Callback builds a table of reports to email mailing.
 *
 * @return
 *   Returns tabular representation of values results.
 */
function ding_dibs_email_reports() {
  $time = REQUEST_TIME - 86400;
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
  if (!$max_count) {
    return '<p>' . t('No transactions found.') . '</p>';
  }
  for ($count = 0; $count < $max_count; $count++) {
    //Clipping of output orders with the status of "1/1/1" and "0/0/0"
    if (!($report[$count]->payment_status == $report[$count]->provider_payment_status && $report[$count]->payment_status == $report[$count]->dibs_capture_status && $report[$count]->provider_payment_status == $report[$count]->dibs_capture_status)) {
      $report_valid[] = $report[$count];
    }
  }
  $header = array(
    array('data' => t('Order ID')),
    array('data' => t('Transaction ID')),
    array('data' => t('Date')),
    array('data' => t('User')),
    array('data' => t('Text')),
    array('data' => t('Amount')),
    array('data' => t('Cleared')),
    array('data' => t('Paid')),
    array('data' => t('Captured')),
  );
  $rows = array();
  foreach ($report_valid as $record) {
    $rows[] = _ding_dibs_report_generation_values($record);
  }

  $output = theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => 'ding_dibs_email_reports'),
  ));
  return $output;
}

/**
 * Callback that builds page reports run cron.
 *
 * @return
 *   Returns tabular representation of values results.
 */
function ding_dibs_cron_report() {
  $report = db_select('ding_dibs_report_cron', 'd')
    ->fields('d', array('id', 'time_start', 'status', 'text'))
    ->orderBy('id', 'DESC')
    ->execute()
    ->fetchAll();
  $max_count = count($report);
  if (!$max_count) {
    return drupal_set_message(t('No cron report.'), 'warning');
  }
  drupal_add_css(drupal_get_path('module', 'ding_dibs') . '/css/ding_dibs.css');
  $header = array(array('data' => t('Message'), 'class' => 'payment'));
  $rows = array();
  foreach ($report as $record) {
    if (empty($record->text)) {
      $day = format_date($record->time_start, 'custom', 'd', date_default_timezone_get());
      $hour = format_date($record->time_start, 'custom', 'H:i', date_default_timezone_get());
      $month = format_date($record->time_start, 'custom', 'F', date_default_timezone_get());
      $record->text = $day . 'th ' . $month . ' ' . $hour . ': Job failed.';
    }
    $row = array(
      array(
        'data' => t($record->text),
        'class' => 'payment',
      ),
    );
    $rows[] = $row;
  }
  $output = theme('table', array(
    'header' => $header,
    'rows' => $rows,
  ));
  return $output;
}

/**
 * The function adds to the dataset for the numbers zero to ten.
 *
 * @param $item
 *   An array of input data for the time period.
 *
 * @return
 *   Returns an array with the format of the time 01, 02.
 */
function _ding_dibs_generate_zero($item) {
  return str_pad($item, 2, "0", STR_PAD_LEFT);
}

/**
 * The function generates a table with data reporting database table
 * so dibs transactions.
 *
 * @param $dibs_transactions_value
 *   Object database table data dibs transactions
 *
 * @param $order_info_long
 *   Long description of table warrant dibs_transactions.
 *
 * @param $status
 *   Status display last column of the table report
 *
 * @param $dibs_transactions_value->payment_order_id
 *   ID orders payment.
 *
 * @param $dibs_transactions_value->payment_transaction_id
 *   ID orders transaction.
 *
 * @param $dibs_transactions_value->payment_time
 *   Time payment.
 *
 * @param $dibs_transactions_value->customer_uid
 *   User ID who has charge.
 *
 * @param $dibs_transactions_value->payment_price
 *   Product price the order in payment.
 *
 * @param $dibs_transactions_value->payment_status
 *   Status payment.
 *
 * @param $dibs_transactions_value->provider_payment_status
 *   Provider payment status.
 *
 * @param $dibs_transactions_value->dibs_capture_status
 *   Captured payment status.
 *
 * @return
 *   Return value table dibs_transactions for mailing reports.
 */
function _ding_dibs_report_generation_values($dibs_transactions_value) {
  $status = $dibs_transactions_value->payment_status . $dibs_transactions_value->provider_payment_status . $dibs_transactions_value->dibs_capture_status;
  if ($status == 100) {
    $status = 'warning';
  }
  else {
    $status = 'error';
  }
  $order_info_long = unserialize($dibs_transactions_value->order_info_long);
  if (is_array($order_info_long) && count($order_info_long) > 0) {
    $text = implode(array_shift($order_info_long)) . ' ';
    $text .= implode(', ', array_map('array_shift', $order_info_long));
  }
  elseif (is_string($order_info_long)) {
    $text = $order_info_long;
  }
  else {
    $text = $dibs_transactions_value->order_info_short;
  }
  $row = array(
    array('data' => check_plain($dibs_transactions_value->payment_order_id)),
    array('data' => check_plain($dibs_transactions_value->payment_transaction_id)),
    array('data' => check_plain($dibs_transactions_value->payment_time)),
    array('data' => check_plain($dibs_transactions_value->customer_uid)),
    array('data' => check_plain($text)),
    array('data' => check_plain($dibs_transactions_value->payment_price)),
    array('data' => $dibs_transactions_value->payment_status ? t('yes') : t('no')),
    array('data' => $dibs_transactions_value->provider_payment_status ? t('yes') : t('no')),
    array('data' => $dibs_transactions_value->dibs_capture_status ? t('yes') : t('no')),
  );
  return $row;
}
