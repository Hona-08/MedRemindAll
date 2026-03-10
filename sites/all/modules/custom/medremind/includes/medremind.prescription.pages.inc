<?php
/**
 * @file
 * User-facing pages for prescriptions.
 */


/**
 * Shows current user's prescriptions.
 */
function medremind_prescriptions_user_page() {
  global $user;

  $output = '<div class="medremind-prescriptions-page">';
  $output .= '<h2>📋 My Prescriptions</h2>';

  // Add prescription button
  $output .= '<div class="dashboard-actions">';
  $output .= l(t('+ Add Prescription'), 'medremind/prescription/add',
    array('attributes' => array('class' => array('medremind-add-btn'))));
  $output .= '</div>';

  // Load user's prescriptions
  $query = new EntityFieldQuery();
  $result = $query->entityCondition('entity_type', 'medremind_prescription')
    ->propertyCondition('uid', $user->uid)
    ->propertyCondition('status', 1)
    ->propertyOrderBy('created', 'DESC')
    ->execute();

  if (!empty($result['medremind_prescription'])) {
    $ids = array_keys($result['medremind_prescription']);
    $prescriptions = entity_load('medremind_prescription', $ids);

    $output .= '<div class="medremind-prescription-list">';

    foreach ($prescriptions as $rx) {
      // Get linked medication name
      $med_name = '';
      if ($rx->med_id && db_table_exists('medremind_medications')) {
        $med_name = db_query('SELECT name FROM {medremind_medications} WHERE med_id = :mid',
          array(':mid' => $rx->med_id))->fetchField();
      }

      $refills_left = max(0, $rx->refills_total - $rx->refills_used);
      $is_expired = ($rx->expiry_date > 0 && $rx->expiry_date < REQUEST_TIME);

      // Status badges
      $status_class = 'rx-active';
      $status_label = 'Active';
      if ($is_expired) {
        $status_class = 'rx-expired';
        $status_label = 'Expired';
      }
      elseif ($refills_left == 0 && $rx->refills_total > 0) {
        $status_class = 'rx-no-refills';
        $status_label = 'No Refills Left';
      }

      $output .= '<div class="medremind-rx-card ' . $status_class . '">';

      // Header row
      $output .= '<div class="rx-card-header">';
      $output .= '<div class="rx-number">Rx# ' . check_plain($rx->prescription_number) . '</div>';
      $output .= '<span class="rx-status-badge ' . $status_class . '">' . $status_label . '</span>';
      $output .= '</div>';

      // Details
      $output .= '<div class="rx-card-body">';

      if ($med_name) {
        $output .= '<div class="rx-detail"><span class="rx-label">💊 Medication:</span> ' . check_plain($med_name) . '</div>';
      }

      $output .= '<div class="rx-detail"><span class="rx-label">👨‍⚕️ Doctor:</span> ' . check_plain($rx->doctor_name) . '</div>';

      if (!empty($rx->pharmacy_name)) {
        $output .= '<div class="rx-detail"><span class="rx-label">🏥 Pharmacy:</span> ' . check_plain($rx->pharmacy_name) . '</div>';
      }

      $output .= '<div class="rx-detail"><span class="rx-label">📅 Prescribed:</span> ' . format_date($rx->prescribed_date, 'custom', 'M j, Y') . '</div>';

      if ($rx->expiry_date > 0) {
        $expiry_class = $is_expired ? 'rx-expired-text' : '';
        $output .= '<div class="rx-detail ' . $expiry_class . '"><span class="rx-label">⏰ Expires:</span> ' . format_date($rx->expiry_date, 'custom', 'M j, Y') . '</div>';
      }

      if ($rx->refills_total > 0) {
        $output .= '<div class="rx-detail"><span class="rx-label">🔄 Refills:</span> ' . $refills_left . ' of ' . $rx->refills_total . ' remaining</div>';

        // Refill progress bar
        $refill_pct = $rx->refills_total > 0 ? round(($refills_left / $rx->refills_total) * 100) : 0;
        $bar_color = $refill_pct > 50 ? '#27ae60' : ($refill_pct > 25 ? '#f39c12' : '#e74c3c');
        $output .= '<div class="rx-refill-bar-bg"><div class="rx-refill-bar-fill" style="width:' . $refill_pct . '%; background:' . $bar_color . ';"></div></div>';
      }

      if (!empty($rx->notes)) {
        $output .= '<div class="rx-detail rx-notes">📝 ' . check_plain($rx->notes) . '</div>';
      }

      $output .= '</div>';

      // Actions
      $output .= '<div class="rx-card-actions">';
      $output .= l(t('Edit'), 'medremind/prescription/' . $rx->prescription_id . '/edit',
        array('attributes' => array('class' => array('rx-link-edit'))));
      $output .= ' | ';
      $output .= l(t('Delete'), 'medremind/prescription/' . $rx->prescription_id . '/delete',
        array('attributes' => array('class' => array('rx-link-delete')),
          'query' => array('destination' => 'medremind/prescriptions')));
      $output .= '</div>';

      $output .= '</div>';
    }

    $output .= '</div>';
  }
  else {
    $output .= '<div class="medremind-empty">';
    $output .= '<div class="empty-icon">📋</div>';
    $output .= '<h4>' . t('No prescriptions yet') . '</h4>';
    $output .= '<p>' . t('Add your first prescription to track refills and expiry dates.') . '</p>';
    $output .= l(t('+ Add Prescription'), 'medremind/prescription/add',
      array('attributes' => array('class' => array('medremind-add-btn'))));
    $output .= '</div>';
  }

  $output .= '</div>';

  return $output;
}


/**
 * Prescription entity form.
 * Called by Entity API for add/edit.
 */
function medremind_prescription_form($form, &$form_state, $prescription = NULL) {
  global $user;

  // If editing, load the entity
  if (!$prescription) {
    $prescription = entity_create('medremind_prescription', array('uid' => $user->uid));
  }

  $form['prescription_number'] = array(
    '#type' => 'textfield',
    '#title' => t('Prescription Number'),
    '#required' => TRUE,
    '#maxlength' => 100,
    '#default_value' => isset($prescription->prescription_number) ? $prescription->prescription_number : '',
    '#description' => t('The Rx number from your prescription label.'),
  );

  // Medication dropdown — load user's medications
  $med_options = array(0 => t('— None —'));
  if (db_table_exists('medremind_medications')) {
    $meds = db_select('medremind_medications', 'm')
      ->fields('m', array('med_id', 'name', 'dosage'))
      ->condition('m.uid', $user->uid)
      ->condition('m.status', MEDREMIND_STATUS_ACTIVE)
      ->orderBy('m.name')
      ->execute();
    foreach ($meds as $med) {
      $med_options[$med->med_id] = $med->name . ' (' . $med->dosage . ')';
    }
  }

  $form['med_id'] = array(
    '#type' => 'select',
    '#title' => t('Linked Medication'),
    '#options' => $med_options,
    '#default_value' => isset($prescription->med_id) ? $prescription->med_id : 0,
    '#description' => t('Link this prescription to one of your medications.'),
  );

  $form['doctor_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Doctor Name'),
    '#required' => TRUE,
    '#maxlength' => 255,
    '#default_value' => isset($prescription->doctor_name) ? $prescription->doctor_name : '',
  );

  $form['pharmacy_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Pharmacy Name'),
    '#maxlength' => 255,
    '#default_value' => isset($prescription->pharmacy_name) ? $prescription->pharmacy_name : '',
  );

  $form['prescribed_date'] = array(
    '#type' => 'date',
    '#title' => t('Prescribed Date'),
    '#required' => TRUE,
    '#default_value' => isset($prescription->prescribed_date) && $prescription->prescribed_date > 0
      ? array('year' => date('Y', $prescription->prescribed_date), 'month' => date('n', $prescription->prescribed_date), 'day' => date('j', $prescription->prescribed_date))
      : array('year' => date('Y'), 'month' => date('n'), 'day' => date('j')),
  );

  $form['has_expiry'] = array(
    '#type' => 'checkbox',
    '#title' => t('Has expiry date'),
    '#default_value' => (isset($prescription->expiry_date) && $prescription->expiry_date > 0) ? 1 : 0,
  );

  $form['expiry_date'] = array(
    '#type' => 'date',
    '#title' => t('Expiry Date'),
    '#default_value' => (isset($prescription->expiry_date) && $prescription->expiry_date > 0)
      ? array('year' => date('Y', $prescription->expiry_date), 'month' => date('n', $prescription->expiry_date), 'day' => date('j', $prescription->expiry_date))
      : array('year' => date('Y', strtotime('+1 year')), 'month' => date('n'), 'day' => date('j')),
    '#states' => array('visible' => array(':input[name="has_expiry"]' => array('checked' => TRUE))),
  );

  $form['refills_total'] = array(
    '#type' => 'select',
    '#title' => t('Total Refills Allowed'),
    '#options' => array(0 => t('No refills')) + drupal_map_assoc(range(1, 20)),
    '#default_value' => isset($prescription->refills_total) ? $prescription->refills_total : 0,
  );

  $form['refills_used'] = array(
    '#type' => 'select',
    '#title' => t('Refills Used'),
    '#options' => drupal_map_assoc(range(0, 20)),
    '#default_value' => isset($prescription->refills_used) ? $prescription->refills_used : 0,
  );

  $form['notes'] = array(
    '#type' => 'textarea',
    '#title' => t('Notes'),
    '#rows' => 3,
    '#default_value' => isset($prescription->notes) ? $prescription->notes : '',
  );

  // Store the entity for submit
  $form['#entity'] = $prescription;

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => isset($prescription->prescription_id) ? t('Update Prescription') : t('Add Prescription'),
    '#attributes' => array('class' => array('medremind-submit-btn')),
  );
  $form['actions']['cancel'] = array(
    '#markup' => ' ' . l(t('Cancel'), 'medremind/prescriptions',
      array('attributes' => array('class' => array('medremind-cancel-btn')))),
  );

  return $form;
}

/**
 * Prescription form validation.
 */
function medremind_prescription_form_validate($form, &$form_state) {
  $v = $form_state['values'];

  if (drupal_strlen($v['prescription_number']) < 2) {
    form_set_error('prescription_number', t('Prescription number must be at least 2 characters.'));
  }

  if (drupal_strlen($v['doctor_name']) < 2) {
    form_set_error('doctor_name', t('Doctor name must be at least 2 characters.'));
  }

  if ($v['refills_used'] > $v['refills_total'] && $v['refills_total'] > 0) {
    form_set_error('refills_used', t('Refills used cannot exceed total refills.'));
  }

  if (!empty($v['has_expiry'])) {
    $prescribed = mktime(0, 0, 0, $v['prescribed_date']['month'], $v['prescribed_date']['day'], $v['prescribed_date']['year']);
    $expiry = mktime(0, 0, 0, $v['expiry_date']['month'], $v['expiry_date']['day'], $v['expiry_date']['year']);
    if ($expiry <= $prescribed) {
      form_set_error('expiry_date', t('Expiry date must be after prescribed date.'));
    }
  }
}

/**
 * Prescription form submit.
 */
function medremind_prescription_form_submit($form, &$form_state) {
  global $user;
  $v = $form_state['values'];
  $prescription = $form['#entity'];

  $prescription->uid = $user->uid;
  $prescription->prescription_number = $v['prescription_number'];
  $prescription->med_id = $v['med_id'];
  $prescription->doctor_name = $v['doctor_name'];
  $prescription->pharmacy_name = $v['pharmacy_name'];
  $prescription->prescribed_date = mktime(0, 0, 0,
    $v['prescribed_date']['month'], $v['prescribed_date']['day'], $v['prescribed_date']['year']);
  $prescription->expiry_date = !empty($v['has_expiry'])
    ? mktime(0, 0, 0, $v['expiry_date']['month'], $v['expiry_date']['day'], $v['expiry_date']['year'])
    : 0;
  $prescription->refills_total = $v['refills_total'];
  $prescription->refills_used = $v['refills_used'];
  $prescription->notes = $v['notes'];
  $prescription->status = 1;
  $prescription->updated = REQUEST_TIME;

  // entity_save() — Entity API handles the INSERT or UPDATE automatically!
  entity_save('medremind_prescription', $prescription);

  $action = isset($prescription->is_new) ? 'added' : 'updated';
  drupal_set_message(t('Prescription @rx @action!', array(
    '@rx' => $prescription->prescription_number,
    '@action' => $action,
  )));

  $form_state['redirect'] = 'medremind/prescriptions';
}