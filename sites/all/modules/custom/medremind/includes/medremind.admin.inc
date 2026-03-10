<?php

/**
 * @file
 * Admin configuration forms for MedRemind.
 * URL: admin/config/medremind
 *
 * Uses system_settings_form() which is a special Drupal helper:
 *   - Auto-saves form values using variable_set()
 *   - Auto-loads saved values using variable_get()
 *   - Adds "Save configuration" button automatically
 *   - The field NAME becomes the variable NAME in database
 *
 * Example:
 *   Field name: 'medremind_site_name'
 *   Saved with: variable_set('medremind_site_name', 'MedRemind')
 *   Loaded with: variable_get('medremind_site_name', 'MedRemind')
 */


/**
 * Admin settings form builder.
 */
function medremind_admin_settings_form($form, &$form_state) {

  // -------------------------------------------
  // SECTION 1: General Settings
  // -------------------------------------------
  $form['general'] = array(
    '#type' => 'fieldset',
    // ↑ fieldset = groups fields in a box with a title
    '#title' => t('General Settings'),
    '#collapsible' => TRUE,
    // ↑ TRUE = admin can click to collapse/expand this section
    '#collapsed' => FALSE,
    // ↑ FALSE = starts open when page loads
  );

  $form['general']['medremind_site_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Application Name'),
    '#description' => t('The name displayed in emails and notifications.'),
    '#default_value' => variable_get('medremind_site_name', 'MedRemind'),
    // ↑ variable_get('name', default_value)
    //   Loads saved value from database
    //   If nothing saved yet, uses 'MedRemind' as default
    //
    //   Because we use system_settings_form(), when admin clicks "Save":
    //   Drupal auto-calls variable_set('medremind_site_name', submitted_value)
    '#required' => TRUE,
    '#maxlength' => 100,
  );

  $form['general']['medremind_max_medications'] = array(
    '#type' => 'select',
    // ↑ Dropdown menu
    '#title' => t('Maximum Medications Per User'),
    '#description' => t('Limit how many medications each user can add.'),
    '#options' => array(
      // ↑ 'stored_value' => 'Display text'
      5 => '5',
      10 => '10',
      15 => '15',
      20 => '20',
      30 => '30',
      50 => '50',
      100 => t('100 (Unlimited)'),
    ),
    '#default_value' => variable_get('medremind_max_medications', 20),
    // ↑ Default: 20 medications per user
  );

  $form['general']['medremind_allow_registration'] = array(
    '#type' => 'checkbox',
    // ↑ On/off toggle, returns 1 or 0
    '#title' => t('Allow new user registrations'),
    '#description' => t('If unchecked, only existing users can access MedRemind.'),
    '#default_value' => variable_get('medremind_allow_registration', 1),
    // ↑ Default: enabled (1)
  );


  // -------------------------------------------
  // SECTION 2: Reminder Settings
  // -------------------------------------------
  $form['reminders'] = array(
    '#type' => 'fieldset',
    '#title' => t('Reminder & Notification Settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['reminders']['medremind_reminder_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable email reminders'),
    '#description' => t('Send email reminders to users before their dose time. Requires cron to be running.'),
    '#default_value' => variable_get('medremind_reminder_enabled', 1),
  );

  $form['reminders']['medremind_default_reminder_time'] = array(
    '#type' => 'select',
    '#title' => t('Default Reminder Time'),
    '#description' => t('How many minutes before dose time to send reminders by default.'),
    '#options' => array(
      5 => t('5 minutes before'),
      10 => t('10 minutes before'),
      15 => t('15 minutes before'),
      30 => t('30 minutes before'),
      60 => t('1 hour before'),
    ),
    '#default_value' => variable_get('medremind_default_reminder_time', 15),
    '#states' => array(
      // ↑ #states = Drupal's conditional visibility
      //   No JavaScript needed! Drupal handles it automatically
      'visible' => array(
        ':input[name="medremind_reminder_enabled"]' => array('checked' => TRUE),
        // ↑ Only show this field when "Enable email reminders" is checked
        //   When unchecked, this field hides automatically
      ),
    ),
  );

  $form['reminders']['medremind_daily_summary_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable daily summary emails'),
    '#description' => t('Send users a daily email summarizing their medication adherence.'),
    '#default_value' => variable_get('medremind_daily_summary_enabled', 1),
  );

  $form['reminders']['medremind_daily_summary_time'] = array(
    '#type' => 'textfield',
    '#title' => t('Daily Summary Send Time'),
    '#description' => t('Time to send daily summary in 24hr format (e.g., 21:00 for 9 PM).'),
    '#default_value' => variable_get('medremind_daily_summary_time', '21:00'),
    '#size' => 10,
    // ↑ Visual width of the input box
    '#maxlength' => 5,
    '#states' => array(
      'visible' => array(
        ':input[name="medremind_daily_summary_enabled"]' => array('checked' => TRUE),
        // ↑ Only show when daily summary checkbox is checked
      ),
    ),
  );


  // -------------------------------------------
  // SECTION 3: Streak & Gamification Settings
  // -------------------------------------------
  $form['streaks'] = array(
    '#type' => 'fieldset',
    '#title' => t('Streak & Gamification'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['streaks']['medremind_streak_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable streak tracking'),
    '#description' => t('Track consecutive days of medication adherence.'),
    '#default_value' => variable_get('medremind_streak_enabled', 1),
  );

  $form['streaks']['medremind_missed_dose_hours'] = array(
    '#type' => 'select',
    '#title' => t('Mark Dose as Missed After'),
    '#description' => t('If user has not taken the dose within this time, mark it as missed.'),
    '#options' => array(
      1 => t('1 hour'),
      2 => t('2 hours'),
      4 => t('4 hours'),
      6 => t('6 hours'),
      12 => t('12 hours'),
      24 => t('End of day'),
    ),
    '#default_value' => variable_get('medremind_missed_dose_hours', 4),
  );


  // -------------------------------------------
  // SECTION 4: Data Management
  // -------------------------------------------
  $form['data'] = array(
    '#type' => 'fieldset',
    '#title' => t('Data Management'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    // ↑ TRUE = starts collapsed (closed) by default
    //   Admin clicks to expand when needed
    //   Good for less frequently used settings
  );

  $form['data']['medremind_log_retention_days'] = array(
    '#type' => 'select',
    '#title' => t('Keep Dose Logs For'),
    '#description' => t('Automatically delete dose logs older than this. Keeps database clean.'),
    '#options' => array(
      30 => t('30 days'),
      60 => t('60 days'),
      90 => t('90 days'),
      180 => t('6 months'),
      365 => t('1 year'),
      0 => t('Forever (no auto-delete)'),
    ),
    '#default_value' => variable_get('medremind_log_retention_days', 365),
  );


  // -------------------------------------------
  // SECTION 5: System Statistics (Read-Only)
  // -------------------------------------------
  $form['stats'] = array(
    '#type' => 'fieldset',
    '#title' => t('System Statistics'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  // Query live stats from database
  $total_users = 0;
  $total_meds = 0;
  $total_doses = 0;
  $today_doses = 0;

  if (db_table_exists('medremind_medications')) {
    // Count total medications across all users
    $total_meds = (int) db_select('medremind_medications', 'm')
      ->countQuery()
      // ↑ SELECT COUNT(*) FROM medremind_medications
      ->execute()
      ->fetchField();

    // Count unique users who have at least one medication
    $total_users = (int) db_query(
      'SELECT COUNT(DISTINCT uid) FROM {medremind_medications}'
      // ↑ COUNT(DISTINCT uid) = counts unique users only
      //   If a user has 5 medications, they still count as 1
    )->fetchField();
  }

  if (db_table_exists('medremind_log')) {
    // Count all dose log entries ever
    $total_doses = (int) db_select('medremind_log', 'l')
      ->countQuery()
      ->execute()
      ->fetchField();

    // Count doses taken today
    $today_doses = (int) db_select('medremind_log', 'l')
      ->condition('l.action', 'taken')
      ->condition('l.action_time', strtotime('today'), '>=')
      // ↑ Only count from midnight today onwards
      ->countQuery()
      ->execute()
      ->fetchField();
  }

  // Display stats as HTML (not editable form fields)
  $form['stats']['info'] = array(
    '#markup' => '<div class="medremind-admin-stats">'
      . '<div class="admin-stat-row">'
      . '  <span class="admin-stat-label">' . t('Total Users') . '</span>'
      . '  <span class="admin-stat-value">' . $total_users . '</span>'
      . '</div>'
      . '<div class="admin-stat-row">'
      . '  <span class="admin-stat-label">' . t('Total Medications') . '</span>'
      . '  <span class="admin-stat-value">' . $total_meds . '</span>'
      . '</div>'
      . '<div class="admin-stat-row">'
      . '  <span class="admin-stat-label">' . t('Total Dose Logs') . '</span>'
      . '  <span class="admin-stat-value">' . $total_doses . '</span>'
      . '</div>'
      . '<div class="admin-stat-row">'
      . '  <span class="admin-stat-label">' . t('Doses Taken Today') . '</span>'
      . '  <span class="admin-stat-value">' . $today_doses . '</span>'
      . '</div>'
      . '</div>',
    // ↑ #markup = raw HTML output inside the form
    //   Not a form field — just for display
    //   Shows live database counts
  );


  // Return as system_settings_form
  return system_settings_form($form);
  // ↑ This is the MAGIC of Drupal admin forms!
  //
  //   system_settings_form() does 3 things automatically:
  //
  //   1. Adds a "Save configuration" submit button
  //   2. On submit, calls variable_set() for EVERY field
  //      Example: variable_set('medremind_site_name', 'MedRemind')
  //   3. On page load, calls variable_get() for default values
  //
  //   You NEVER need to write a submit handler!
  //   Just define fields and Drupal saves them.
  //
  //   The field NAME = the variable NAME in database
  //   So 'medremind_max_medications' field saves to
  //   variable_get('medremind_max_medications')
}


/**
 * Validation for admin settings.
 *
 * Function naming: form_name + _validate
 * Drupal auto-discovers this by naming convention.
 */
function medremind_admin_settings_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  // ↑ All submitted form values

  // Validate daily summary time format (HH:MM)
  if (!empty($values['medremind_daily_summary_enabled'])) {
    $time = $values['medremind_daily_summary_time'];
    if (!preg_match('/^([01][0-9]|2[0-3]):[0-5][0-9]$/', $time)) {
      // ↑ Same regex we used in the medication form
      //   Valid: 08:00, 14:30, 21:00, 23:59
      //   Invalid: 25:00, 8:00, abc
      form_set_error('medremind_daily_summary_time',
        t('Daily summary time must be in HH:MM format (e.g., 21:00).'));
      // ↑ Highlights the field in red, shows error message
      //   Prevents form from saving until fixed
    }
  }
}