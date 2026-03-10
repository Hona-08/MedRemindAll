<?php

/**
 * @file
 * Block content builders for MedRemind.
 * Blocks are small widgets that show in sidebars.
 */

/**
 * Build content for the "Next Dose" sidebar block.
 */
function _medremind_block_next_dose() {
  global $user;

  $block = array();

  if (!user_is_logged_in()) {
    return $block;
  }

  if (!db_table_exists('medremind_medications')) {
    return $block;
  }

  $result = db_select('medremind_medications', 'm')
    ->fields('m')
    ->condition('m.uid', $user->uid)
    ->condition('m.status', MEDREMIND_STATUS_ACTIVE)
    ->execute();

  $upcoming = array();
  $now = date('H:i');

  foreach ($result as $med) {
    $times = drupal_json_decode($med->times);
    if (empty($times) || !is_array($times)) {
      continue;
    }

    sort($times);
    foreach ($times as $time) {
      if ($time > $now) {
        $upcoming[] = array(
          'med_id' => $med->med_id,
          'name' => $med->name,
          'dosage' => $med->dosage,
          'time' => $time,
          'time_formatted' => _medremind_block_format_time($time),
          'minutes_until' => _medremind_minutes_until($time),
        );
        break;
      }
    }
  }

  usort($upcoming, function ($a, $b) {
    return strcmp($a['time'], $b['time']);
  });

  $block['subject'] = t('💊 Next Dose');

  if (empty($upcoming)) {
    $total_meds = (int) db_select('medremind_medications', 'm')
      ->condition('m.uid', $user->uid)
      ->condition('m.status', MEDREMIND_STATUS_ACTIVE)
      ->countQuery()
      ->execute()
      ->fetchField();

    if ($total_meds > 0) {
      $taken_today = 0;
      if (db_table_exists('medremind_log')) {
        $taken_today = (int) db_select('medremind_log', 'l')
          ->condition('l.uid', $user->uid)
          ->condition('l.action', MEDREMIND_ACTION_TAKEN)
          ->condition('l.action_time', strtotime('today'), '>=')
          ->countQuery()
          ->execute()
          ->fetchField();
      }

      $block['content'] = '<div class="medremind-block-empty">'
        . '<p class="block-done-icon">✅</p>'
        . '<p>' . t('All done for today!') . '</p>'
        . '<p class="block-mini-stat">'
        . t('@taken of @total taken', array(
            '@taken' => $taken_today,
            '@total' => $total_meds,
          ))
        . '</p>'
        . '<div class="block-dashboard-link">'
        . l(t('View Dashboard →'), 'medremind', array(
            'attributes' => array('class' => array('block-link')),
          ))
        . '</div>'
        . '</div>';
    }
    else {
      $block['content'] = '<div class="medremind-block-empty">'
        . '<p class="block-done-icon">💊</p>'
        . '<p>' . t('No medications added yet.') . '</p>'
        . '<div class="block-dashboard-link">'
        . l(t('+ Add Medication'), 'medremind/add', array(
            'attributes' => array('class' => array('block-link')),
          ))
        . '</div>'
        . '</div>';
    }
  }
  else {
    $output = '<div class="medremind-block-list">';

    $count = 0;
    foreach ($upcoming as $dose) {
      if ($count >= 3) break;

      $urgent_class = '';
      if ($dose['minutes_until'] <= 30) {
        $urgent_class = 'block-dose-urgent';
      }

      $output .= '<div class="block-dose-item ' . $urgent_class . '">';
      $output .= '  <div class="block-dose-time">' . check_plain($dose['time_formatted']) . '</div>';
      $output .= '  <div class="block-dose-info">';
      $output .= '    <strong>' . check_plain($dose['name']) . '</strong>';
      $output .= '    <span class="block-dose-dosage">' . check_plain($dose['dosage']) . '</span>';

      if ($dose['minutes_until'] <= 60) {
        $output .= '    <span class="block-dose-countdown">'
          . t('in @min min', array('@min' => $dose['minutes_until']))
          . '</span>';
      }

      $output .= '  </div>';
      $output .= '</div>';

      $count++;
    }

    $output .= '</div>';

    $output .= '<div class="block-dashboard-link">';
    $output .= l(t('View Dashboard →'), 'medremind', array(
      'attributes' => array('class' => array('block-link')),
    ));
    $output .= '</div>';

    $block['content'] = $output;
  }

  return $block;
}


/**
 * Build content for the "Quick Stats" block.
 */
function _medremind_block_quick_stats() {
  global $user;

  $block = array();

  if (!user_is_logged_in() || !db_table_exists('medremind_medications')) {
    return $block;
  }

  $total = (int) db_select('medremind_medications', 'm')
    ->condition('m.uid', $user->uid)
    ->condition('m.status', MEDREMIND_STATUS_ACTIVE)
    ->countQuery()
    ->execute()
    ->fetchField();

  $taken = 0;
  if (db_table_exists('medremind_log')) {
    $today_start = strtotime('today');
    $taken = (int) db_select('medremind_log', 'l')
      ->condition('l.uid', $user->uid)
      ->condition('l.action', MEDREMIND_ACTION_TAKEN)
      ->condition('l.action_time', $today_start, '>=')
      ->countQuery()
      ->execute()
      ->fetchField();
  }

  $progress = 0;
  if ($total > 0) {
    $progress = min(round(($taken / $total) * 100), 100);
  }

  $block['subject'] = t('📊 Today');

  $output = '<div class="medremind-block-stats">';

  $output .= '<div class="block-progress-bar">';
  $output .= '  <div class="block-progress-fill" style="width: ' . $progress . '%;"></div>';
  $output .= '</div>';
  $output .= '<p class="block-progress-text">'
    . t('@taken of @total taken (@pct%)', array(
        '@taken' => $taken,
        '@total' => $total,
        '@pct' => $progress,
      ))
    . '</p>';

  if (db_table_exists('medremind_streaks')) {
    $best_streak = (int) db_select('medremind_streaks', 's')
      ->fields('s', array('current_streak'))
      ->condition('s.uid', $user->uid)
      ->orderBy('s.current_streak', 'DESC')
      ->range(0, 1)
      ->execute()
      ->fetchField();

    if ($best_streak > 0) {
      $output .= '<p class="block-streak">🔥 '
        . t('@days day streak', array('@days' => $best_streak))
        . '</p>';
    }
  }

  $output .= '</div>';

  $block['content'] = $output;

  return $block;
}


/**
 * Convert 24hr time to 12hr format.
 */
function _medremind_block_format_time($time_24) {
  $timestamp = strtotime($time_24);
  if ($timestamp === FALSE) {
    return $time_24;
  }
  return date('g:i A', $timestamp);
}

/**
 * Calculate minutes until a given time today.
 */
function _medremind_minutes_until($time_24) {
  $dose_timestamp = strtotime('today ' . $time_24);
  $now = REQUEST_TIME;
  $diff = $dose_timestamp - $now;

  if ($diff <= 0) {
    return 0;
  }

  return round($diff / 60);
}
/**
 * Block content: Notification bell.
 * Shows unread count badge and dropdown.
 */
function _medremind_block_notifications() {
  global $user;

  module_load_include('inc', 'medremind', 'includes/medremind.notifications');

  $count = medremind_notification_unread_count($user->uid);
  $notifications = medremind_notification_get($user->uid, 5, TRUE);

  $output = '<div class="medremind-notif-bell-wrapper">';

  // Bell icon with count badge
  $output .= '<a href="' . url('medremind/notifications') . '" class="medremind-notif-bell" id="medremind-notif-bell">';
  $output .= '🔔';
  if ($count > 0) {
    $output .= '<span class="medremind-notif-badge" id="medremind-notif-badge">' . $count . '</span>';
  }
  $output .= '</a>';

  // Dropdown preview (hidden by default, shown on hover/click via CSS)
  $output .= '<div class="medremind-notif-dropdown" id="medremind-notif-dropdown">';

  if (empty($notifications)) {
    $output .= '<div class="medremind-notif-empty">No new notifications</div>';
  }
  else {
    foreach ($notifications as $notif) {
      $type_icons = array(
        'reminder' => '💊',
        'missed' => '❌',
        'streak' => '🔥',
        'achievement' => '🏆',
      );
      $icon = isset($type_icons[$notif->type]) ? $type_icons[$notif->type] : '🔔';
      $time_ago = format_interval(REQUEST_TIME - $notif->created) . ' ago';

      $output .= '<div class="medremind-notif-dropdown-item">';
      $output .= '  <span class="medremind-notif-dropdown-icon">' . $icon . '</span>';
      $output .= '  <div class="medremind-notif-dropdown-text">';
      $output .= '    <strong>' . check_plain($notif->title) . '</strong>';
      $output .= '    <small>' . $time_ago . '</small>';
      $output .= '  </div>';
      $output .= '</div>';
    }
  }

  $output .= '<a href="' . url('medremind/notifications') . '" class="medremind-notif-view-all">View All</a>';
  $output .= '</div>';

  $output .= '</div>';

  return $output;
}