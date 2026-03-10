<?php
/**
 * @file
 * Notification system for MedRemind.
 */

function medremind_notification_create($uid, $type, $title, $message, $med_id = NULL) {
  if (!db_table_exists('medremind_notifications')) return 0;
  return db_insert('medremind_notifications')->fields(array(
    'uid' => $uid, 'type' => $type, 'title' => $title,
    'message' => $message, 'med_id' => $med_id,
    'is_read' => 0, 'created' => REQUEST_TIME,
  ))->execute();
}

function medremind_notify_dose_reminder($uid, $med_name, $dosage, $med_id) {
  medremind_notification_create($uid, 'reminder',
    t('Time for @med', array('@med' => $med_name)),
    t('Take your @dosage dose of @med now.', array('@dosage' => $dosage, '@med' => $med_name)),
    $med_id);
}

function medremind_notify_missed_dose($uid, $med_name, $time, $med_id) {
  medremind_notification_create($uid, 'missed',
    t('Missed: @med', array('@med' => $med_name)),
    t('You missed your @time dose of @med.', array('@time' => $time, '@med' => $med_name)),
    $med_id);
}

function medremind_notify_streak($uid, $med_name, $streak_count, $med_id) {
  $milestones = array(7, 14, 21, 30, 60, 90, 100, 180, 365);
  if (!in_array($streak_count, $milestones)) return;

  $emojis = array(7=>'🔥', 14=>'⭐', 21=>'🏆', 30=>'👑', 60=>'💎', 90=>'🚀', 100=>'💯', 180=>'🎯', 365=>'🏅');
  $emoji = isset($emojis[$streak_count]) ? $emojis[$streak_count] : '🎉';

  medremind_notification_create($uid, 'streak',
    t('@emoji @count-day streak!', array('@emoji' => $emoji, '@count' => $streak_count)),
    t('Amazing! @count-day streak for @med!', array('@count' => $streak_count, '@med' => $med_name)),
    $med_id);
}

function medremind_notification_unread_count($uid) {
  if (!db_table_exists('medremind_notifications')) return 0;
  return (int) db_select('medremind_notifications', 'n')
    ->condition('n.uid', $uid)->condition('n.is_read', 0)
    ->countQuery()->execute()->fetchField();
}

function medremind_notification_get($uid, $limit = 20, $unread_only = FALSE) {
  if (!db_table_exists('medremind_notifications')) return array();
  $query = db_select('medremind_notifications', 'n')->fields('n')
    ->condition('n.uid', $uid)->orderBy('n.created', 'DESC')->range(0, $limit);
  if ($unread_only) $query->condition('n.is_read', 0);
  return $query->execute()->fetchAll();
}

function medremind_notification_mark_all_read($uid) {
  if (!db_table_exists('medremind_notifications')) return;
  db_update('medremind_notifications')->fields(array('is_read' => 1))
    ->condition('uid', $uid)->condition('is_read', 0)->execute();
}

function medremind_notification_cleanup($days = 30) {
  if (!db_table_exists('medremind_notifications')) return;
  db_delete('medremind_notifications')
    ->condition('created', REQUEST_TIME - ($days * 86400), '<')->execute();
}

function medremind_notifications_page() {
  global $user;
  medremind_notification_mark_all_read($user->uid);
  $notifications = medremind_notification_get($user->uid, 50);

  $output = '<div class="medremind-notifications-page">';
  $output .= '<h2>Notifications</h2>';

  if (empty($notifications)) {
    $output .= '<div class="medremind-empty"><p>🔔 No notifications yet.</p></div>';
  } else {
    $icons = array('reminder' => '💊', 'missed' => '❌', 'streak' => '🔥', 'achievement' => '🏆');
    foreach ($notifications as $notif) {
      $icon = isset($icons[$notif->type]) ? $icons[$notif->type] : '🔔';
      $time_ago = format_interval(REQUEST_TIME - $notif->created) . ' ago';
      $read_class = $notif->is_read ? 'read' : 'unread';

      $output .= '<div class="medremind-notification-item medremind-notif-' . $read_class . ' medremind-notif-type-' . check_plain($notif->type) . '">';
      $output .= '<div class="medremind-notif-icon">' . $icon . '</div>';
      $output .= '<div class="medremind-notif-content">';
      $output .= '<div class="medremind-notif-title">' . check_plain($notif->title) . '</div>';
      $output .= '<div class="medremind-notif-message">' . check_plain($notif->message) . '</div>';
      $output .= '<div class="medremind-notif-time">' . $time_ago . '</div>';
      $output .= '</div></div>';
    }
  }

  $output .= '</div>';
  return $output;
}

function medremind_notifications_mark_read_ajax() {
  global $user;
  medremind_notification_mark_all_read($user->uid);
  drupal_json_output(array('status' => 'ok', 'count' => 0));
  drupal_exit();
}

function medremind_notifications_count_ajax() {
  global $user;
  $count = medremind_notification_unread_count($user->uid);
  drupal_json_output(array('status' => 'ok', 'count' => $count));
  drupal_exit();
}