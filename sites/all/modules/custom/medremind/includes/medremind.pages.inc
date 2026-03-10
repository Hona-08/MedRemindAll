<?php
/**
 * @file
 * Page callbacks: Dashboard and History.
 */

function medremind_dashboard_page() {
  global $user;
  $meds = array();
  $stats = array('total' => 0, 'taken' => 0, 'missed' => 0, 'streak' => 0);

  if (db_table_exists('medremind_medications')) {
    $result = db_select('medremind_medications', 'm')->fields('m')
      ->condition('m.uid', $user->uid)->condition('m.status', MEDREMIND_STATUS_ACTIVE)
      ->orderBy('m.created', 'DESC')->execute();

    foreach ($result as $med) {
      $med->next_dose = _medremind_next_dose($med);
      $med->streak = _medremind_streak($med->med_id, $user->uid);
      $med->taken_today = _medremind_taken_today($med->med_id, $user->uid);
      $meds[] = $med;
    }
    $stats['total'] = count($meds);

    $today = strtotime('today');
    if (db_table_exists('medremind_log')) {
      $stats['taken'] = (int) db_select('medremind_log', 'l')
        ->condition('l.uid', $user->uid)->condition('l.action', MEDREMIND_ACTION_TAKEN)
        ->condition('l.action_time', $today, '>=')->countQuery()->execute()->fetchField();
      $stats['missed'] = (int) db_select('medremind_log', 'l')
        ->condition('l.uid', $user->uid)->condition('l.action', MEDREMIND_ACTION_MISSED)
        ->condition('l.scheduled_time', $today, '>=')->countQuery()->execute()->fetchField();
    }
    if (db_table_exists('medremind_streaks')) {
      $stats['streak'] = (int) db_select('medremind_streaks', 's')
        ->fields('s', array('current_streak'))->condition('s.uid', $user->uid)
        ->orderBy('s.current_streak', 'DESC')->range(0, 1)->execute()->fetchField();
    }
  }

  return theme('medremind_dashboard', array(
    'medications' => $meds, 'stats' => $stats, 'user_name' => format_username($user),
  ));
}

function medremind_history_page() {
  global $user;
  $logs = array();
  $adherence = 0;
  $month = format_date(REQUEST_TIME, 'custom', 'F Y');
  $month_start = strtotime('first day of this month midnight');

  if (db_table_exists('medremind_log')) {
    // COUNT total for pager
    $total = (int) db_select('medremind_log', 'l')
      ->condition('l.uid', $user->uid)
      ->condition('l.scheduled_time', $month_start, '>=')
      ->countQuery()->execute()->fetchField();

    // PAGER — 10 items per page
    $per_page = 10;
    $current_page = pager_default_initialize($total, $per_page);

    $result = db_select('medremind_log', 'l')->fields('l')
      ->condition('l.uid', $user->uid)
      ->condition('l.scheduled_time', $month_start, '>=')
      ->orderBy('l.action_time', 'DESC')
      ->range($current_page * $per_page, $per_page)
      ->execute();

    foreach ($result as $log) {
      $log->med_name = db_query('SELECT name FROM {medremind_medications} WHERE med_id = :m',
        array(':m' => $log->med_id))->fetchField() ?: t('Unknown');
      $logs[] = $log;
    }

    $taken = (int) db_select('medremind_log', 'l')
      ->condition('l.uid', $user->uid)
      ->condition('l.action', MEDREMIND_ACTION_TAKEN)
      ->condition('l.scheduled_time', $month_start, '>=')
      ->countQuery()->execute()->fetchField();
    if ($total > 0) $adherence = round(($taken / $total) * 100);
  }

  return theme('medremind_history', array(
    'logs' => $logs,
    'month' => $month,
    'adherence' => $adherence,
  ));
}

function _medremind_next_dose($med) {
  $times = drupal_json_decode($med->times);
  if (empty($times)) return t('No schedule');
  $now = date('H:i');
  sort($times);
  foreach ($times as $t) { if ($t > $now) return date('g:i A', strtotime($t)); }
  return t('Done for today ✓');
}

function _medremind_streak($med_id, $uid) {
  if (!db_table_exists('medremind_streaks')) return 0;
  return (int) db_select('medremind_streaks', 's')->fields('s', array('current_streak'))
    ->condition('s.med_id', $med_id)->condition('s.uid', $uid)->execute()->fetchField();
}

function _medremind_taken_today($med_id, $uid) {
  if (!db_table_exists('medremind_log')) return FALSE;
  return (int) db_select('medremind_log', 'l')
    ->condition('l.med_id', $med_id)->condition('l.uid', $uid)
    ->condition('l.action', MEDREMIND_ACTION_TAKEN)
    ->condition('l.action_time', strtotime('today'), '>=')
    ->countQuery()->execute()->fetchField() > 0;
}