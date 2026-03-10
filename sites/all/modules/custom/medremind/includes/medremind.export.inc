<?php
/**
 * @file
 * Export and import functionality for MedRemind.
 *
 * Export: Medications CSV, History CSV, Full data CSV
 * Import: Medications from CSV with validation
 * Template: Downloadable sample CSV for import
 */
/**
 * Page callback: /medremind/export
 */
function medremind_export_page() {
  $output = '<div class="medremind-export-page">';
  $output .= '<h2>📤 Export Your Data</h2>';
  $output .= '<p>Download your medication data in CSV format.</p>';

  $output .= '<div class="medremind-export-options">';

  // Card 1: Medications
  $output .= '<div class="medremind-export-card">';
  $output .= '<div class="medremind-export-icon">💊</div>';
  $output .= '<h3>Medications</h3>';
  $output .= '<p>All medications with dosage, frequency, and schedule.</p>';
  $output .= '<a href="' . url('medremind/export/medications/csv') . '" class="medremind-btn medremind-btn-primary">Download CSV</a>';
  $output .= '</div>';

  // Card 2: History
  $output .= '<div class="medremind-export-card">';
  $output .= '<div class="medremind-export-icon">📋</div>';
  $output .= '<h3>Dose History</h3>';
  $output .= '<p>Complete dose log with dates, actions, and notes.</p>';
  $output .= '<a href="' . url('medremind/export/history/csv') . '" class="medremind-btn medremind-btn-primary">Download CSV</a>';
  $output .= '</div>';

  // Card 3: Full Export
  $output .= '<div class="medremind-export-card">';
  $output .= '<div class="medremind-export-icon">📦</div>';
  $output .= '<h3>Full Data Export</h3>';
  $output .= '<p>Everything: medications, history, streaks, and settings.</p>';
  $output .= '<a href="' . url('medremind/export/full/csv') . '" class="medremind-btn medremind-btn-primary">Download CSV</a>';
  $output .= '</div>';

  $output .= '</div>';

  // Import section
  $output .= '<h2 style="margin-top: 40px;">📥 Import Data</h2>';
  $output .= '<p>Import medications from a CSV file. ';
  $output .= '<a href="' . url('medremind/export/template/csv') . '">Download sample template</a>.</p>';
  $import_form = drupal_get_form('medremind_import_form');
  $output .= drupal_render($import_form);
  $output .= '</div>';

  return $output;
}


/**
 * Export medications as CSV.
 */
function medremind_export_medications_csv() {
  global $user;

  $medications = db_select('medremind_medications', 'm')
    ->fields('m')
    ->condition('m.uid', $user->uid)
    ->orderBy('m.name')
    ->execute()
    ->fetchAll();

  drupal_add_http_header('Content-Type', 'text/csv; charset=utf-8');
  drupal_add_http_header('Content-Disposition', 'attachment; filename="medremind-medications-' . date('Y-m-d') . '.csv"');

  $handle = fopen('php://output', 'w');

  fputcsv($handle, array('Name', 'Dosage', 'Frequency', 'Times', 'Start Date', 'End Date', 'Status', 'Notes', 'Created'));

  foreach ($medications as $med) {
    $status_label = ($med->status == MEDREMIND_STATUS_ACTIVE) ? 'Active' : 'Paused';
    $start = $med->start_date ? format_date($med->start_date, 'custom', 'Y-m-d') : '';
    $end = $med->end_date ? format_date($med->end_date, 'custom', 'Y-m-d') : '';
    $created = format_date($med->created, 'custom', 'Y-m-d H:i');

    fputcsv($handle, array(
      $med->name, $med->dosage, $med->frequency, $med->times,
      $start, $end, $status_label, $med->notes, $created,
    ));
  }

  fclose($handle);
  drupal_exit();
}


/**
 * Export dose history as CSV.
 */
function medremind_export_history_csv() {
  global $user;

  $logs = db_query(
    'SELECT l.*, m.name AS med_name, m.dosage
     FROM {medremind_log} l
     INNER JOIN {medremind_medications} m ON l.med_id = m.med_id
     WHERE l.uid = :uid
     ORDER BY l.action_time DESC',
    array(':uid' => $user->uid)
  )->fetchAll();

  drupal_add_http_header('Content-Type', 'text/csv; charset=utf-8');
  drupal_add_http_header('Content-Disposition', 'attachment; filename="medremind-history-' . date('Y-m-d') . '.csv"');

  $handle = fopen('php://output', 'w');

  fputcsv($handle, array('Date', 'Time', 'Medication', 'Dosage', 'Action', 'Scheduled Time', 'Notes'));

  foreach ($logs as $log) {
    $action_date = format_date($log->action_time, 'custom', 'Y-m-d');
    $action_time = format_date($log->action_time, 'custom', 'H:i');
    $scheduled = $log->scheduled_time ? format_date($log->scheduled_time, 'custom', 'H:i') : '';

    fputcsv($handle, array(
      $action_date, $action_time, $log->med_name, $log->dosage,
      ucfirst($log->action), $scheduled, $log->notes,
    ));
  }

  fclose($handle);
  drupal_exit();
}


/**
 * Export all data as CSV.
 */
function medremind_export_full_csv() {
  global $user;

  drupal_add_http_header('Content-Type', 'text/csv; charset=utf-8');
  drupal_add_http_header('Content-Disposition', 'attachment; filename="medremind-full-export-' . date('Y-m-d') . '.csv"');

  $handle = fopen('php://output', 'w');

  // Section: Medications
  fputcsv($handle, array('=== MEDICATIONS ==='));
  fputcsv($handle, array('Name', 'Dosage', 'Frequency', 'Times', 'Start Date', 'Status', 'Notes'));

  if (db_table_exists('medremind_medications')) {
    $medications = db_select('medremind_medications', 'm')->fields('m')
      ->condition('m.uid', $user->uid)->execute();

    foreach ($medications as $med) {
      fputcsv($handle, array(
        $med->name, $med->dosage, $med->frequency, $med->times,
        $med->start_date ? format_date($med->start_date, 'custom', 'Y-m-d') : '',
        $med->status == MEDREMIND_STATUS_ACTIVE ? 'Active' : 'Paused',
        $med->notes,
      ));
    }
  }

  fputcsv($handle, array(''));

  // Section: History
  fputcsv($handle, array('=== DOSE HISTORY ==='));
  fputcsv($handle, array('Date', 'Medication', 'Action', 'Notes'));

  if (db_table_exists('medremind_log')) {
    $logs = db_query(
      'SELECT l.*, m.name AS med_name
       FROM {medremind_log} l
       INNER JOIN {medremind_medications} m ON l.med_id = m.med_id
       WHERE l.uid = :uid ORDER BY l.action_time DESC',
      array(':uid' => $user->uid)
    );

    foreach ($logs as $log) {
      fputcsv($handle, array(
        format_date($log->action_time, 'custom', 'Y-m-d H:i'),
        $log->med_name, ucfirst($log->action), $log->notes,
      ));
    }
  }

  fputcsv($handle, array(''));

  // Section: Streaks
  if (db_table_exists('medremind_streaks')) {
    fputcsv($handle, array('=== STREAKS ==='));
    fputcsv($handle, array('Medication', 'Current Streak', 'Best Streak'));

    $streaks = db_query(
      'SELECT s.*, m.name AS med_name
       FROM {medremind_streaks} s
       INNER JOIN {medremind_medications} m ON s.med_id = m.med_id
       WHERE s.uid = :uid',
      array(':uid' => $user->uid)
    );

    foreach ($streaks as $s) {
      fputcsv($handle, array($s->med_name, $s->current_streak, $s->best_streak));
    }
  }

  fclose($handle);
  drupal_exit();
}


/**
 * Download sample import template.
 */
function medremind_export_template_csv() {
  drupal_add_http_header('Content-Type', 'text/csv; charset=utf-8');
  drupal_add_http_header('Content-Disposition', 'attachment; filename="medremind-import-template.csv"');

  $handle = fopen('php://output', 'w');

  fputcsv($handle, array('Name', 'Dosage', 'Frequency', 'Times', 'Notes'));
  fputcsv($handle, array('Paracetamol', '500mg', 'twice_daily', '["08:00","20:00"]', 'Take after meals'));
  fputcsv($handle, array('Vitamin D', '1000IU', 'once_daily', '["09:00"]', 'Take with breakfast'));
  fputcsv($handle, array('Amoxicillin', '250mg', 'three_daily', '["08:00","14:00","20:00"]', '7-day course'));

  fclose($handle);
  drupal_exit();
}


/**
 * Import form.
 */
function medremind_import_form($form, &$form_state) {
  $form['csv_file'] = array(
    '#type' => 'file',
    '#title' => t('CSV File'),
    '#description' => t('Upload a CSV with columns: Name, Dosage, Frequency, Times (JSON), Notes.'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import Medications'),
    '#attributes' => array('class' => array('medremind-btn', 'medremind-btn-primary')),
  );

  return $form;
}

/**
 * Import form validation.
 */
function medremind_import_form_validate($form, &$form_state) {
  $file = file_save_upload('csv_file', array(
    'file_validate_extensions' => array('csv'),
  ));

  if ($file) {
    $form_state['values']['csv_file'] = $file;
  }
  else {
    form_set_error('csv_file', t('Please upload a valid CSV file.'));
  }
}

/**
 * Import form submit.
 */
function medremind_import_form_submit($form, &$form_state) {
  global $user;

  $file = $form_state['values']['csv_file'];
  $filepath = drupal_realpath($file->uri);

  $handle = fopen($filepath, 'r');
  if (!$handle) {
    drupal_set_message(t('Could not read the file.'), 'error');
    return;
  }

  // Skip header row
  fgetcsv($handle);

  $imported = 0;
  $skipped = 0;
  $max_meds = variable_get('medremind_max_medications', 20);

  $current_count = (int) db_select('medremind_medications', 'm')
    ->condition('m.uid', $user->uid)
    ->condition('m.status', MEDREMIND_STATUS_ACTIVE)
    ->countQuery()->execute()->fetchField();

  while (($row = fgetcsv($handle)) !== FALSE) {
    // Skip empty rows and section headers
    if (empty($row[0]) || strpos($row[0], '===') !== FALSE) {
      continue;
    }

    // Check max limit
    if (($current_count + $imported) >= $max_meds) {
      $skipped++;
      continue;
    }

    $name = isset($row[0]) ? trim($row[0]) : '';
    $dosage = isset($row[1]) ? trim($row[1]) : '';
    $frequency = isset($row[2]) ? trim($row[2]) : 'once_daily';
    $times = isset($row[3]) ? trim($row[3]) : '["08:00"]';
    $notes = isset($row[4]) ? trim($row[4]) : '';

    // Validate name
    if (drupal_strlen($name) < 2) {
      $skipped++;
      continue;
    }

    // Validate times JSON
    $decoded_times = drupal_json_decode($times);
    if (!is_array($decoded_times)) {
      $times = '["08:00"]';
      $decoded_times = array('08:00');
    }

    // Check for duplicate name
    $exists = db_select('medremind_medications', 'm')
      ->condition('m.uid', $user->uid)
      ->condition('m.name', $name)
      ->countQuery()->execute()->fetchField();

    if ($exists) {
      $skipped++;
      continue;
    }

    // Insert medication
    $med_id = db_insert('medremind_medications')->fields(array(
      'uid' => $user->uid,
      'name' => $name,
      'dosage' => $dosage,
      'frequency' => $frequency,
      'times' => $times,
      'start_date' => REQUEST_TIME,
      'end_date' => 0,
      'notes' => $notes,
      'status' => MEDREMIND_STATUS_ACTIVE,
      'created' => REQUEST_TIME,
      'updated' => REQUEST_TIME,
    ))->execute();

    // Create schedule entries
    foreach ($decoded_times as $t) {
      db_insert('medremind_schedule')->fields(array(
        'med_id' => $med_id,
        'uid' => $user->uid,
        'dose_time' => $t,
        'days_of_week' => 'all',
        'status' => 1,
      ))->execute();
    }

    // Create streak record
    if (db_table_exists('medremind_streaks')) {
      db_insert('medremind_streaks')->fields(array(
        'uid' => $user->uid,
        'med_id' => $med_id,
        'current_streak' => 0,
        'best_streak' => 0,
        'last_taken' => 0,
      ))->execute();
    }

    $imported++;
  }

  fclose($handle);
  file_delete($file);

  drupal_set_message(t('Import complete: @imported medications imported, @skipped skipped.', array(
    '@imported' => $imported,
    '@skipped' => $skipped,
  )));

  $form_state['redirect'] = 'medremind';
}