<?php

/**
 * @file
 * Cron tasks for MedRemind.
 *
 * HOW DRUPAL CRON WORKS:
 *   1. Drupal runs cron on a schedule (every 1-3 hours)
 *   2. It calls hook_cron() in every enabled module
 *   3. Our hook_cron() in medremind.module calls this file
 *   4. We check for pending tasks and process them
 *
 * TO TEST MANUALLY:
 *   Visit: /admin/config/system/cron → Click "Run cron"
 *   Or visit: /cron.php?cron_key=YOUR_KEY
 *
 * IMPORTANT: Cron has a time limit (usually 240 seconds)
 *   So we process in batches, not everything at once
 */


/**
 * Main cron processor — called by hook_cron().
 *
 * This is the entry point. It runs all sub-tasks:
 *   1. Generate reminders for upcoming doses
 *   2. Send pending reminders via email
 *   3. Auto-mark missed doses
 *   4. Send daily summary emails
 *   5. Clean up old log entries
 */
function _medremind_process_reminders() {

  // Task 1: Generate reminders for upcoming doses
  _medremind_generate_reminders();

  // Task 2: Send pending email reminders
  _medremind_send_pending_reminders();

  // Task 3: Auto-mark missed doses
  _medremind_mark_missed_doses();

  // Task 4: Send daily summary (if it's the right time)
  _medremind_send_daily_summaries();

  // Task 5: Clean up old logs
  _medremind_cleanup_old_logs();
}


// ============================================
// TASK 1: GENERATE REMINDERS
// ============================================

/**
 * Check all medications and create reminders for upcoming doses.
 *
 * Logic:
 *   - Look at each user's medications
 *   - Check each dose time
 *   - If dose is coming up within the reminder window
 *   - AND we haven't already created a reminder for it
 *   - Create a reminder entry in medremind_reminders table
 */
function _medremind_generate_reminders() {

  // Check if reminders are enabled globally
  if (!variable_get('medremind_reminder_enabled', 1)) {
    return;
    // ↑ Admin turned off reminders — do nothing
  }

  if (!db_table_exists('medremind_medications') || !db_table_exists('medremind_reminders')) {
    return;
  }

  $now = REQUEST_TIME;
  // ↑ Current Unix timestamp

  // Default: remind 15 minutes before dose
  $default_before = variable_get('medremind_default_reminder_time', 15);

  // Load all active medications
  $medications = db_select('medremind_medications', 'm')
    ->fields('m')
    ->condition('m.status', MEDREMIND_STATUS_ACTIVE)
    ->execute();

  foreach ($medications as $med) {
    // Decode the times JSON: ["08:00", "14:00", "20:00"]
    $times = drupal_json_decode($med->times);
    if (empty($times) || !is_array($times)) {
      continue;
      // ↑ Skip medications with no schedule
    }

    // Check user's personal reminder preference
    $user_before = $default_before;
    if (db_table_exists('medremind_settings')) {
      $user_setting = db_query(
        'SELECT reminder_email, reminder_before FROM {medremind_settings} WHERE uid = :uid',
        array(':uid' => $med->uid)
      )->fetchObject();

      if ($user_setting) {
        // User turned off email reminders
        if (!$user_setting->reminder_email) {
          continue;
          // ↑ This user doesn't want email reminders — skip
        }
        $user_before = $user_setting->reminder_before;
        // ↑ Use user's custom "remind me X minutes before" setting
      }
    }

    // Check each dose time
    foreach ($times as $time_str) {
      // Convert "08:00" to today's timestamp
      // strtotime('today 08:00') = today at 8 AM
      $dose_timestamp = strtotime('today ' . $time_str);

      if ($dose_timestamp === FALSE || $dose_timestamp < $now) {
        continue;
        // ↑ Skip past dose times (already happened today)
      }

      // Calculate when reminder should be sent
      // If dose is at 08:00 and reminder is 15 min before:
      // reminder_time = 08:00 - (15 * 60) = 07:45
      $reminder_time = $dose_timestamp - ($user_before * 60);

      // Is the reminder time within the next 2 hours?
      // We don't want to create reminders too far in advance
      if ($reminder_time > $now && $reminder_time <= ($now + 7200)) {

        // Check if we already created this reminder today
        $already_exists = db_select('medremind_reminders', 'r')
          ->condition('r.uid', $med->uid)
          ->condition('r.med_id', $med->med_id)
          ->condition('r.reminder_time', $reminder_time)
          ->countQuery()
          ->execute()
          ->fetchField();

        if (!$already_exists) {
          // Create the reminder!
          db_insert('medremind_reminders')
            ->fields(array(
              'uid' => $med->uid,
              'med_id' => $med->med_id,
              'reminder_time' => $reminder_time,
              'method' => 'email',
              'sent' => 0,
              // ↑ 0 = not sent yet, will be sent in Task 2
              'created' => $now,
            ))
            ->execute();

          watchdog('medremind', 'Reminder created for user @uid, med @med at @time.', array(
            '@uid' => $med->uid,
            '@med' => $med->name,
            '@time' => format_date($reminder_time, 'short'),
          ));
        }
      }
    }
  }
}


// ============================================
// TASK 2: SEND PENDING REMINDERS
// ============================================

/**
 * Send all pending email reminders.
 *
 * Finds reminders where:
 *   - sent = 0 (not yet sent)
 *   - reminder_time <= now (time has arrived)
 * Sends email and marks as sent.
 */
function _medremind_send_pending_reminders() {

  if (!db_table_exists('medremind_reminders')) {
    return;
  }

  $now = REQUEST_TIME;

  // Find unsent reminders that are due
  $reminders = db_select('medremind_reminders', 'r')
    ->fields('r')
    ->condition('r.sent', 0)
    ->condition('r.reminder_time', $now, '<=')
    // ↑ Only reminders whose time has arrived
    ->range(0, 50)
    // ↑ Process max 50 per cron run (avoid timeout)
    ->execute();

  $count = 0;

  foreach ($reminders as $reminder) {
    // Load medication details
    $med = db_query(
      'SELECT name, dosage FROM {medremind_medications} WHERE med_id = :mid',
      array(':mid' => $reminder->med_id)
    )->fetchObject();

    if (!$med) {
      // Medication was deleted — mark reminder as sent (skip it)
      db_update('medremind_reminders')
        ->fields(array('sent' => 1))
        ->condition('reminder_id', $reminder->reminder_id)
        ->execute();
      continue;
    }

    // Load user account (for email address)
    $account = user_load($reminder->uid);
    if (!$account || empty($account->mail)) {
      continue;
    }

    // Send the email!
    $params = array(
      'med_name' => $med->name,
      'med_dosage' => $med->dosage,
      'account' => $account,
      'type' => 'dose_reminder',
    );

    $result = drupal_mail(
      'medremind',
      // ↑ Module name — Drupal looks for medremind_mail()
      'dose_reminder',
      // ↑ Email key — identifies which email template to use
      $account->mail,
      // ↑ Recipient email address
      user_preferred_language($account),
      // ↑ User's language preference
      $params
      // ↑ Data to use in the email template
    );

    // Mark as sent
    if ($result['result']) {
      db_update('medremind_reminders')
        ->fields(array('sent' => 1))
        ->condition('reminder_id', $reminder->reminder_id)
        ->execute();

      $count++;
    }
  }

  if ($count > 0) {
    watchdog('medremind', 'Cron: sent @count dose reminders.', array(
      '@count' => $count,
    ));
  }
}


// ============================================
// TASK 3: AUTO-MARK MISSED DOSES
// ============================================

/**
 * Mark doses as missed if the time window has passed.
 *
 * Example: If "Mark as missed after 4 hours" is set,
 *   and dose was at 08:00 and it's now 12:01,
 *   and user never took it → mark as MISSED
 */
function _medremind_mark_missed_doses() {

  if (!db_table_exists('medremind_medications') || !db_table_exists('medremind_log')) {
    return;
  }

  $now = REQUEST_TIME;
  $missed_hours = variable_get('medremind_missed_dose_hours', 4);
  // ↑ From admin settings: how many hours before marking missed
  $threshold = $missed_hours * 3600;
  // ↑ Convert hours to seconds (4 hours = 14400 seconds)

  // Load all active medications
  $medications = db_select('medremind_medications', 'm')
    ->fields('m')
    ->condition('m.status', MEDREMIND_STATUS_ACTIVE)
    ->execute();

  foreach ($medications as $med) {
    $times = drupal_json_decode($med->times);
    if (empty($times) || !is_array($times)) {
      continue;
    }

    foreach ($times as $time_str) {
      // Get today's dose timestamp
      $dose_timestamp = strtotime('today ' . $time_str);

      if ($dose_timestamp === FALSE) {
        continue;
      }

      // Has the missed threshold passed?
      // dose at 08:00 + 4 hours = 12:00
      // If now > 12:00, check if taken
      if (($dose_timestamp + $threshold) < $now && $dose_timestamp > strtotime('today')) {

        // Check if already logged (taken, skipped, or missed)
        $already_logged = db_select('medremind_log', 'l')
          ->condition('l.med_id', $med->med_id)
          ->condition('l.uid', $med->uid)
          ->condition('l.scheduled_time', $dose_timestamp)
          ->countQuery()
          ->execute()
          ->fetchField();

        if (!$already_logged) {
          // No action taken — mark as MISSED
          db_insert('medremind_log')
            ->fields(array(
              'med_id' => $med->med_id,
              'uid' => $med->uid,
              'scheduled_time' => $dose_timestamp,
              'action' => MEDREMIND_ACTION_MISSED,
              'action_time' => $now,
              'notes' => 'Auto-marked as missed by system.',
            ))
            ->execute();

          // Reset streak
          _medremind_cron_reset_streak($med->med_id, $med->uid);

          watchdog('medremind', 'Auto-missed: @name for user @uid at @time.', array(
            '@name' => $med->name,
            '@uid' => $med->uid,
            '@time' => $time_str,
          ));
        }
      }
    }
  }
}

/**
 * Reset streak (cron version — can't use ajax.inc helpers).
 */
function _medremind_cron_reset_streak($med_id, $uid) {
  if (db_table_exists('medremind_streaks')) {
    db_update('medremind_streaks')
      ->fields(array('current_streak' => 0))
      ->condition('med_id', $med_id)
      ->condition('uid', $uid)
      ->execute();
  }
}


// ============================================
// TASK 4: DAILY SUMMARY EMAILS
// ============================================

/**
 * Send daily summary emails to users.
 *
 * Runs at the admin-configured time (e.g., 21:00 / 9 PM).
 * Summarizes: what was taken, missed, skipped today.
 */
function _medremind_send_daily_summaries() {

  // Is daily summary enabled?
  if (!variable_get('medremind_daily_summary_enabled', 1)) {
    return;
  }

  // What time should summaries be sent?
  $summary_time = variable_get('medremind_daily_summary_time', '21:00');
  $summary_timestamp = strtotime('today ' . $summary_time);

  $now = REQUEST_TIME;

  // Only send if current time is within 1 hour AFTER the summary time
  // This prevents sending multiple times if cron runs frequently
  if ($now < $summary_timestamp || $now > ($summary_timestamp + 3600)) {
    return;
    // ↑ Not the right time yet, or already past the window
  }

  // Check if we already sent summaries today
  // We use a variable to track this
  $last_summary = variable_get('medremind_last_summary_date', '');
  $today = date('Y-m-d');

  if ($last_summary == $today) {
    return;
    // ↑ Already sent today — don't send again
  }

  if (!db_table_exists('medremind_medications') || !db_table_exists('medremind_log')) {
    return;
  }

  // Find all users who have medications
  $users = db_query('SELECT DISTINCT uid FROM {medremind_medications} WHERE status = :status', array(
    ':status' => MEDREMIND_STATUS_ACTIVE,
  ));

  $count = 0;
  $today_start = strtotime('today');

  foreach ($users as $row) {
    // Check if user wants daily summary
    $wants_summary = TRUE;
    if (db_table_exists('medremind_settings')) {
      $setting = db_query(
        'SELECT daily_summary FROM {medremind_settings} WHERE uid = :uid',
        array(':uid' => $row->uid)
      )->fetchField();

      if ($setting !== FALSE && !$setting) {
        $wants_summary = FALSE;
      }
    }

    if (!$wants_summary) {
      continue;
    }

    $account = user_load($row->uid);
    if (!$account || empty($account->mail)) {
      continue;
    }

    // Gather today's stats for this user
    $total_meds = (int) db_select('medremind_medications', 'm')
      ->condition('m.uid', $row->uid)
      ->condition('m.status', MEDREMIND_STATUS_ACTIVE)
      ->countQuery()
      ->execute()
      ->fetchField();

    $taken = (int) db_select('medremind_log', 'l')
      ->condition('l.uid', $row->uid)
      ->condition('l.action', MEDREMIND_ACTION_TAKEN)
      ->condition('l.action_time', $today_start, '>=')
      ->countQuery()
      ->execute()
      ->fetchField();

    $missed = (int) db_select('medremind_log', 'l')
      ->condition('l.uid', $row->uid)
      ->condition('l.action', MEDREMIND_ACTION_MISSED)
      ->condition('l.action_time', $today_start, '>=')
      ->countQuery()
      ->execute()
      ->fetchField();

    $skipped = (int) db_select('medremind_log', 'l')
      ->condition('l.uid', $row->uid)
      ->condition('l.action', MEDREMIND_ACTION_SKIPPED)
      ->condition('l.action_time', $today_start, '>=')
      ->countQuery()
      ->execute()
      ->fetchField();

    // Send summary email
    $params = array(
      'account' => $account,
      'type' => 'daily_summary',
      'total_meds' => $total_meds,
      'taken' => $taken,
      'missed' => $missed,
      'skipped' => $skipped,
      'date' => format_date($now, 'custom', 'l, F j, Y'),
    );

    drupal_mail('medremind', 'daily_summary', $account->mail,
      user_preferred_language($account), $params);

    $count++;
  }

  // Mark that we sent summaries today
  variable_set('medremind_last_summary_date', $today);

  if ($count > 0) {
    watchdog('medremind', 'Cron: sent @count daily summaries.', array(
      '@count' => $count,
    ));
  }
}


// ============================================
// TASK 5: CLEANUP OLD LOGS
// ============================================

/**
 * Delete old dose log entries based on retention setting.
 *
 * If admin set "Keep logs for 90 days":
 *   Delete any log entry older than 90 days
 *   This keeps the database from growing forever
 */
function _medremind_cleanup_old_logs() {

  $retention_days = variable_get('medremind_log_retention_days', 365);

  if ($retention_days == 0) {
    return;
    // ↑ 0 = keep forever — do nothing
  }

  if (!db_table_exists('medremind_log')) {
    return;
  }

  // Calculate cutoff timestamp
  // If retention = 90 days: delete anything before 90 days ago
  $cutoff = REQUEST_TIME - ($retention_days * 86400);
  // ↑ 86400 = seconds in one day (60 * 60 * 24)

  $deleted = db_delete('medremind_log')
    ->condition('action_time', $cutoff, '<')
    // ↑ Delete where action_time is LESS THAN cutoff
    ->execute();
  // ↑ Returns number of rows deleted

  if ($deleted > 0) {
    watchdog('medremind', 'Cron: cleaned up @count old log entries (older than @days days).', array(
      '@count' => $deleted,
      '@days' => $retention_days,
    ));
  }

  // Also clean up old sent reminders (older than 7 days)
  if (db_table_exists('medremind_reminders')) {
    $reminder_cutoff = REQUEST_TIME - (7 * 86400);

    db_delete('medremind_reminders')
      ->condition('sent', 1)
      // ↑ Only delete SENT reminders
      ->condition('created', $reminder_cutoff, '<')
      ->execute();
  }
}