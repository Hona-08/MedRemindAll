<?php
/**
 * @file
 * Forms: Add/Edit medication, Delete, User settings.
 */

function medremind_medication_form($form, &$form_state, $med_id = NULL) {
  global $user;
  $med = NULL;
  if ($med_id) {
    $med = db_select('medremind_medications', 'm')->fields('m')
      ->condition('m.med_id', $med_id)->condition('m.uid', $user->uid)
      ->execute()->fetchObject();
    if (!$med) { drupal_set_message(t('Not found.'), 'error'); drupal_goto('medremind'); }
  }

  $times = array('08:00');
  if ($med && !empty($med->times)) {
    $d = drupal_json_decode($med->times);
    if (is_array($d)) $times = $d;
  }

  $form['med_id'] = array('#type' => 'hidden', '#value' => $med_id);
  $form['name'] = array('#type' => 'textfield', '#title' => t('Medication Name'),
    '#required' => TRUE, '#maxlength' => 255, '#default_value' => $med ? $med->name : '');
  $form['dosage'] = array('#type' => 'textfield', '#title' => t('Dosage'),
    '#required' => TRUE, '#maxlength' => 100, '#default_value' => $med ? $med->dosage : '');
  $form['frequency'] = array('#type' => 'select', '#title' => t('Frequency'),
    '#required' => TRUE, '#default_value' => $med ? $med->frequency : 'once_daily',
    '#options' => array('once_daily' => t('Once Daily'), 'twice_daily' => t('Twice Daily'),
      'three_daily' => t('3x Daily'), 'four_daily' => t('4x Daily'),
      'weekly' => t('Weekly'), 'as_needed' => t('As Needed')));

  $form['times_wrapper'] = array('#type' => 'fieldset', '#title' => t('Dose Times'),
    '#description' => t('24-hour format: 08:00, 14:30'));
  $labels = array('Morning', 'Afternoon', 'Evening', 'Night');
  for ($i = 0; $i < 4; $i++) {
    $form['times_wrapper']['time_' . ($i+1)] = array('#type' => 'textfield',
      '#title' => t('Time @n (@label)', array('@n' => $i+1, '@label' => $labels[$i])),
      '#size' => 10, '#maxlength' => 5,
      '#default_value' => isset($times[$i]) ? $times[$i] : '',
      '#required' => ($i == 0));
  }

  $form['start_date'] = array('#type' => 'date', '#title' => t('Start Date'), '#required' => TRUE,
    '#default_value' => ($med && $med->start_date)
      ? array('year' => date('Y', $med->start_date), 'month' => date('n', $med->start_date), 'day' => date('j', $med->start_date))
      : array('year' => date('Y'), 'month' => date('n'), 'day' => date('j')));
  $form['has_end_date'] = array('#type' => 'checkbox', '#title' => t('Has end date'),
    '#default_value' => ($med && $med->end_date > 0) ? 1 : 0);
  $form['end_date'] = array('#type' => 'date', '#title' => t('End Date'),
    '#default_value' => array('year' => date('Y'), 'month' => date('n'), 'day' => date('j')),
    '#states' => array('visible' => array(':input[name="has_end_date"]' => array('checked' => TRUE))));
  $form['notes'] = array('#type' => 'textarea', '#title' => t('Notes'), '#rows' => 3,
    '#default_value' => $med ? $med->notes : '');

  $form['submit'] = array('#type' => 'submit',
    '#value' => $med ? t('Update Medication') : t('Add Medication'),
    '#attributes' => array('class' => array('medremind-submit-btn')));
  $form['cancel'] = array('#markup' => ' ' . l(t('Cancel'), 'medremind',
    array('attributes' => array('class' => array('medremind-cancel-btn')))));
  return $form;
}

function medremind_medication_form_validate($form, &$form_state) {
  $v = $form_state['values'];
  if (drupal_strlen($v['name']) < 2)
    form_set_error('name', t('Name must be at least 2 characters.'));

  $has_time = FALSE;
  for ($i = 1; $i <= 4; $i++) {
    $t = trim($v['time_' . $i]);
    if (!empty($t)) {
      if (!preg_match('/^([01][0-9]|2[0-3]):[0-5][0-9]$/', $t))
        form_set_error('time_' . $i, t('Time @i must be HH:MM format.', array('@i' => $i)));
      $has_time = TRUE;
    }
  }
  if (!$has_time) form_set_error('time_1', t('At least one dose time is required.'));

  if (!empty($v['has_end_date'])) {
    $s = mktime(0, 0, 0, $v['start_date']['month'], $v['start_date']['day'], $v['start_date']['year']);
    $e = mktime(0, 0, 0, $v['end_date']['month'], $v['end_date']['day'], $v['end_date']['year']);
    if ($e <= $s) form_set_error('end_date', t('End date must be after start date.'));
  }

  if (empty($v['med_id'])) {
    global $user;
    $max = variable_get('medremind_max_medications', 20);
    $count = db_select('medremind_medications', 'm')->condition('m.uid', $user->uid)
      ->countQuery()->execute()->fetchField();
    if ($count >= $max) form_set_error('name', t('Maximum @max medications reached.', array('@max' => $max)));
  }
}

function medremind_medication_form_submit($form, &$form_state) {
  global $user;
  $v = $form_state['values'];

  $times = array();
  for ($i = 1; $i <= 4; $i++) { $t = trim($v['time_' . $i]); if (!empty($t)) $times[] = $t; }

  $data = array(
    'uid' => $user->uid, 'name' => $v['name'], 'dosage' => $v['dosage'],
    'frequency' => $v['frequency'], 'times' => drupal_json_encode($times),
    'start_date' => mktime(0,0,0, $v['start_date']['month'], $v['start_date']['day'], $v['start_date']['year']),
    'end_date' => !empty($v['has_end_date']) ? mktime(0,0,0, $v['end_date']['month'], $v['end_date']['day'], $v['end_date']['year']) : 0,
    'notes' => $v['notes'], 'status' => MEDREMIND_STATUS_ACTIVE, 'updated' => REQUEST_TIME,
  );

  if ($v['med_id']) {
    db_update('medremind_medications')->fields($data)
      ->condition('med_id', $v['med_id'])->condition('uid', $user->uid)->execute();
    drupal_set_message(t('%name updated.', array('%name' => $v['name'])));
  } else {
    $data['created'] = REQUEST_TIME;
    $mid = db_insert('medremind_medications')->fields($data)->execute();
    foreach ($times as $t) {
      db_insert('medremind_schedule')->fields(array(
        'med_id' => $mid, 'uid' => $user->uid, 'dose_time' => $t, 'days_of_week' => 'all', 'status' => 1))->execute();
    }
    db_insert('medremind_streaks')->fields(array(
      'uid' => $user->uid, 'med_id' => $mid, 'current_streak' => 0, 'best_streak' => 0, 'last_taken' => 0))->execute();
    drupal_set_message(t('%name added!', array('%name' => $v['name'])));
  }
  $form_state['redirect'] = 'medremind';
}

function medremind_delete_confirm_form($form, &$form_state, $med_id) {
  global $user;
  $med = db_select('medremind_medications', 'm')->fields('m', array('name', 'med_id'))
    ->condition('m.med_id', $med_id)->condition('m.uid', $user->uid)->execute()->fetchObject();
  if (!$med) { drupal_set_message(t('Not found.'), 'error'); drupal_goto('medremind'); }
  $form['med_id'] = array('#type' => 'hidden', '#value' => $med->med_id);
  return confirm_form($form, t('Delete %name?', array('%name' => $med->name)), 'medremind',
    t('This permanently deletes the medication and all its history.'), t('Delete'), t('Cancel'));
}

function medremind_delete_confirm_form_submit($form, &$form_state) {
  global $user;
  $mid = $form_state['values']['med_id'];
  $name = db_query('SELECT name FROM {medremind_medications} WHERE med_id = :m', array(':m' => $mid))->fetchField();
  foreach (array('medremind_log','medremind_schedule','medremind_reminders','medremind_streaks') as $t) {
    db_delete($t)->condition('med_id', $mid)->condition('uid', $user->uid)->execute();
  }
  db_delete('medremind_medications')->condition('med_id', $mid)->condition('uid', $user->uid)->execute();
  drupal_set_message(t('%name deleted.', array('%name' => $name)));
  $form_state['redirect'] = 'medremind';
}

function medremind_user_settings_form($form, &$form_state) {
  global $user;
  $s = db_select('medremind_settings', 's')->fields('s')
    ->condition('s.uid', $user->uid)->execute()->fetchObject();

  $form['reminder_email'] = array('#type' => 'checkbox', '#title' => t('Enable email reminders'),
    '#default_value' => $s ? $s->reminder_email : 1);
  $form['reminder_before'] = array('#type' => 'select', '#title' => t('Remind me before'),
    '#options' => array(5 => '5 min', 10 => '10 min', 15 => '15 min', 30 => '30 min', 60 => '1 hour'),
    '#default_value' => $s ? $s->reminder_before : 15);
  $form['timezone'] = array('#type' => 'select', '#title' => t('Timezone'),
    '#options' => system_time_zones(), '#default_value' => $s ? $s->timezone : drupal_get_user_timezone());
  $form['daily_summary'] = array('#type' => 'checkbox', '#title' => t('Daily summary email'),
    '#default_value' => $s ? $s->daily_summary : 1);
  $form['submit'] = array('#type' => 'submit', '#value' => t('Save Settings'),
    '#attributes' => array('class' => array('medremind-submit-btn')));
  return $form;
}

function medremind_user_settings_form_submit($form, &$form_state) {
  global $user;
  $v = $form_state['values'];
  $data = array('uid' => $user->uid, 'reminder_email' => $v['reminder_email'],
    'reminder_before' => $v['reminder_before'], 'timezone' => $v['timezone'],
    'daily_summary' => $v['daily_summary']);
  $exists = db_select('medremind_settings', 's')->fields('s', array('uid'))
    ->condition('s.uid', $user->uid)->execute()->fetchField();
  if ($exists) db_update('medremind_settings')->fields($data)->condition('uid', $user->uid)->execute();
  else db_insert('medremind_settings')->fields($data)->execute();
  drupal_set_message(t('Settings saved.'));
}