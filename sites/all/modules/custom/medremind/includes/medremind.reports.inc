<?php
/**
 * @file
 * Reports and analytics for MedRemind.
 *
 * Provides:
 *   - Weekly/Monthly/All-time adherence overview
 *   - Daily adherence bar chart (pure CSS)
 *   - Per-medication breakdown with progress bars
 *   - Streak leaderboard
 */


/**
 * Page callback: /medremind/reports
 */
function medremind_reports_page() {
  global $user;

  $output = '<div class="medremind-reports-page">';
  $output .= '<h2>📊 Your Medication Reports</h2>';

  // Period selector tabs
  $period = isset($_GET['period']) ? check_plain($_GET['period']) : 'month';

  $output .= '<div class="medremind-report-tabs">';
  $output .= '<a href="?period=week" class="medremind-report-tab ' . ($period == 'week' ? 'active' : '') . '">This Week</a>';
  $output .= '<a href="?period=month" class="medremind-report-tab ' . ($period == 'month' ? 'active' : '') . '">This Month</a>';
  $output .= '<a href="?period=all" class="medremind-report-tab ' . ($period == 'all' ? 'active' : '') . '">All Time</a>';
  $output .= '</div>';

  // Calculate date range based on selected period
  switch ($period) {
    case 'week':
      $start = strtotime('monday this week');
      $period_label = 'This Week';
      break;
    case 'all':
      $start = 0;
      $period_label = 'All Time';
      break;
    default:
      $start = strtotime('first day of this month');
      $period_label = date('F Y');
      break;
  }

  // Section 1: Overview Cards
  $output .= _medremind_report_overview($user->uid, $start, $period_label);

  // Section 2: Daily Adherence Chart
  $output .= _medremind_report_daily_chart($user->uid, $start, $period);

  // Section 3: Per-Medication Breakdown
  $output .= _medremind_report_medication_breakdown($user->uid, $start);

  // Section 4: Streak Stats
  $output .= _medremind_report_streaks($user->uid);

  $output .= '</div>';

  return $output;
}


/**
 * Section 1: Overview statistics cards.
 */
function _medremind_report_overview($uid, $start, $period_label) {
  $taken = 0;
  $missed = 0;
  $skipped = 0;

  if (db_table_exists('medremind_log')) {
    $query_taken = db_select('medremind_log', 'l')
      ->condition('l.uid', $uid)
      ->condition('l.action', MEDREMIND_ACTION_TAKEN);
    if ($start > 0) $query_taken->condition('l.action_time', $start, '>=');
    $taken = (int) $query_taken->countQuery()->execute()->fetchField();

    $query_missed = db_select('medremind_log', 'l')
      ->condition('l.uid', $uid)
      ->condition('l.action', MEDREMIND_ACTION_MISSED);
    if ($start > 0) $query_missed->condition('l.action_time', $start, '>=');
    $missed = (int) $query_missed->countQuery()->execute()->fetchField();

    $query_skipped = db_select('medremind_log', 'l')
      ->condition('l.uid', $uid)
      ->condition('l.action', MEDREMIND_ACTION_SKIPPED);
    if ($start > 0) $query_skipped->condition('l.action_time', $start, '>=');
    $skipped = (int) $query_skipped->countQuery()->execute()->fetchField();
  }

  $total = $taken + $missed + $skipped;
  $adherence = $total > 0 ? round(($taken / $total) * 100) : 0;

  if ($adherence >= 90) {
    $color = '#27ae60';
    $label = 'Excellent';
  }
  elseif ($adherence >= 75) {
    $color = '#f39c12';
    $label = 'Good';
  }
  elseif ($adherence >= 50) {
    $color = '#e67e22';
    $label = 'Needs Improvement';
  }
  else {
    $color = '#e74c3c';
    $label = 'Critical';
  }

  $output = '<div class="medremind-report-section">';
  $output .= '<h3>' . check_plain($period_label) . ' Overview</h3>';
  $output .= '<div class="medremind-report-cards">';

  // Big adherence circle
  $output .= '<div class="medremind-report-card medremind-report-card-big">';
  $output .= '<div class="medremind-adherence-circle" style="border-color: ' . $color . ';">';
  $output .= '<span class="medremind-adherence-number" style="color: ' . $color . ';">' . $adherence . '%</span>';
  $output .= '</div>';
  $output .= '<div class="medremind-adherence-label" style="color: ' . $color . ';">' . $label . '</div>';
  $output .= '<div class="medremind-report-card-title">Adherence Rate</div>';
  $output .= '</div>';

  // Taken
  $output .= '<div class="medremind-report-card">';
  $output .= '<div class="medremind-report-card-number" style="color: #27ae60;">✅ ' . $taken . '</div>';
  $output .= '<div class="medremind-report-card-title">Doses Taken</div>';
  $output .= '</div>';

  // Missed
  $output .= '<div class="medremind-report-card">';
  $output .= '<div class="medremind-report-card-number" style="color: #e74c3c;">❌ ' . $missed . '</div>';
  $output .= '<div class="medremind-report-card-title">Doses Missed</div>';
  $output .= '</div>';

  // Skipped
  $output .= '<div class="medremind-report-card">';
  $output .= '<div class="medremind-report-card-number" style="color: #f39c12;">⏭️ ' . $skipped . '</div>';
  $output .= '<div class="medremind-report-card-title">Doses Skipped</div>';
  $output .= '</div>';

  // Total
  $output .= '<div class="medremind-report-card">';
  $output .= '<div class="medremind-report-card-number" style="color: #667eea;">📋 ' . $total . '</div>';
  $output .= '<div class="medremind-report-card-title">Total Doses</div>';
  $output .= '</div>';

  $output .= '</div>';
  $output .= '</div>';

  return $output;
}


/**
 * Section 2: Daily adherence bar chart (pure CSS).
 */
function _medremind_report_daily_chart($uid, $start, $period) {
  $output = '<div class="medremind-report-section">';
  $output .= '<h3>📈 Daily Adherence</h3>';

  if ($period == 'week') {
    $days = 7;
    $chart_start = strtotime('monday this week');
  }
  elseif ($period == 'all') {
    $days = 30;
    $chart_start = strtotime('-29 days');
  }
  else {
    $days = (int) date('j');
    $chart_start = strtotime('first day of this month');
  }

  // Gather data for each day
  $daily_data = array();
  $max_doses = 1;

  for ($i = 0; $i < $days; $i++) {
    $day_start = $chart_start + ($i * 86400);
    $day_end = $day_start + 86400;

    $day_taken = 0;
    $day_missed = 0;

    if (db_table_exists('medremind_log')) {
      $day_taken = (int) db_select('medremind_log', 'l')
        ->condition('l.uid', $uid)
        ->condition('l.action', MEDREMIND_ACTION_TAKEN)
        ->condition('l.action_time', $day_start, '>=')
        ->condition('l.action_time', $day_end, '<')
        ->countQuery()->execute()->fetchField();

      $day_missed = (int) db_select('medremind_log', 'l')
        ->condition('l.uid', $uid)
        ->condition('l.action', MEDREMIND_ACTION_MISSED)
        ->condition('l.action_time', $day_start, '>=')
        ->condition('l.action_time', $day_end, '<')
        ->countQuery()->execute()->fetchField();
    }

    $day_total = $day_taken + $day_missed;
    if ($day_total > $max_doses) {
      $max_doses = $day_total;
    }

    $daily_data[] = array(
      'date' => $day_start,
      'taken' => $day_taken,
      'missed' => $day_missed,
      'total' => $day_total,
    );
  }

  // Render chart bars
  $output .= '<div class="medremind-chart">';

  foreach ($daily_data as $day) {
    $taken_height = $max_doses > 0 ? round(($day['taken'] / $max_doses) * 100) : 0;
    $missed_height = $max_doses > 0 ? round(($day['missed'] / $max_doses) * 100) : 0;
    $day_num = date('j', $day['date']);
    $day_name = date('D', $day['date']);
    $is_today = (date('Y-m-d', $day['date']) == date('Y-m-d'));

    $output .= '<div class="medremind-chart-bar-group ' . ($is_today ? 'today' : '') . '">';
    $output .= '<div class="medremind-chart-bars" style="height: 120px;">';

    if ($day['total'] > 0) {
      $output .= '<div class="medremind-chart-bar taken" style="height: ' . $taken_height . '%;"></div>';
      if ($day['missed'] > 0) {
        $output .= '<div class="medremind-chart-bar missed" style="height: ' . $missed_height . '%;"></div>';
      }
    }
    else {
      $output .= '<div class="medremind-chart-bar empty" style="height: 5%;"></div>';
    }

    $output .= '</div>';
    $output .= '<div class="medremind-chart-label">' . $day_num . '</div>';
    if ($period == 'week') {
      $output .= '<div class="medremind-chart-sublabel">' . $day_name . '</div>';
    }
    $output .= '</div>';
  }

  $output .= '</div>';

  // Legend
  $output .= '<div class="medremind-chart-legend">';
  $output .= '<span class="medremind-legend-item"><span class="medremind-legend-color taken"></span> Taken</span>';
  $output .= '<span class="medremind-legend-item"><span class="medremind-legend-color missed"></span> Missed</span>';
  $output .= '</div>';

  $output .= '</div>';

  return $output;
}


/**
 * Section 3: Per-medication breakdown.
 */
function _medremind_report_medication_breakdown($uid, $start) {
  $output = '<div class="medremind-report-section">';
  $output .= '<h3>💊 Medication Breakdown</h3>';

  if (!db_table_exists('medremind_medications')) {
    $output .= '<p>No medications to show.</p></div>';
    return $output;
  }

  $medications = db_select('medremind_medications', 'm')
    ->fields('m', array('med_id', 'name', 'dosage'))
    ->condition('m.uid', $uid)
    ->condition('m.status', MEDREMIND_STATUS_ACTIVE)
    ->execute();

  $has_meds = FALSE;
  $output .= '<div class="medremind-report-breakdown">';

  foreach ($medications as $med) {
    $has_meds = TRUE;

    $med_taken = 0;
    $med_total = 0;

    if (db_table_exists('medremind_log')) {
      $query_taken = db_select('medremind_log', 'l')
        ->condition('l.med_id', $med->med_id)
        ->condition('l.action', MEDREMIND_ACTION_TAKEN);
      if ($start > 0) $query_taken->condition('l.action_time', $start, '>=');
      $med_taken = (int) $query_taken->countQuery()->execute()->fetchField();

      $query_total = db_select('medremind_log', 'l')
        ->condition('l.med_id', $med->med_id);
      if ($start > 0) $query_total->condition('l.action_time', $start, '>=');
      $med_total = (int) $query_total->countQuery()->execute()->fetchField();
    }

    $pct = $med_total > 0 ? round(($med_taken / $med_total) * 100) : 0;

    if ($pct >= 90) {
      $bar_color = '#27ae60';
    }
    elseif ($pct >= 75) {
      $bar_color = '#f39c12';
    }
    else {
      $bar_color = '#e74c3c';
    }

    $output .= '<div class="medremind-breakdown-item">';
    $output .= '<div class="medremind-breakdown-header">';
    $output .= '<span class="medremind-breakdown-name">' . check_plain($med->name) . ' <small>' . check_plain($med->dosage) . '</small></span>';
    $output .= '<span class="medremind-breakdown-pct" style="color: ' . $bar_color . ';">' . $pct . '%</span>';
    $output .= '</div>';
    $output .= '<div class="medremind-breakdown-bar-bg">';
    $output .= '<div class="medremind-breakdown-bar-fill" style="width: ' . $pct . '%; background: ' . $bar_color . ';"></div>';
    $output .= '</div>';
    $output .= '<div class="medremind-breakdown-detail">' . $med_taken . ' of ' . $med_total . ' doses taken</div>';
    $output .= '</div>';
  }

  if (!$has_meds) {
    $output .= '<p>No medications to show.</p>';
  }

  $output .= '</div>';
  $output .= '</div>';

  return $output;
}


/**
 * Section 4: Streak leaderboard.
 */
function _medremind_report_streaks($uid) {
  if (!db_table_exists('medremind_streaks') || !db_table_exists('medremind_medications')) {
    return '';
  }

  $output = '<div class="medremind-report-section">';
  $output .= '<h3>🔥 Streak Stats</h3>';

  $streaks = db_query(
    'SELECT s.current_streak, s.best_streak, m.name
     FROM {medremind_streaks} s
     INNER JOIN {medremind_medications} m ON s.med_id = m.med_id
     WHERE s.uid = :uid AND m.status = :status
     ORDER BY s.current_streak DESC',
    array(':uid' => $uid, ':status' => MEDREMIND_STATUS_ACTIVE)
  );

  $output .= '<div class="medremind-streak-table">';
  $output .= '<div class="medremind-streak-header">';
  $output .= '<span>Medication</span>';
  $output .= '<span>Current Streak</span>';
  $output .= '<span>Best Streak</span>';
  $output .= '</div>';

  $has_streaks = FALSE;

  foreach ($streaks as $s) {
    $has_streaks = TRUE;

    $fire = '';
    if ($s->current_streak >= 30) {
      $fire = '👑 ';
    }
    elseif ($s->current_streak >= 14) {
      $fire = '⭐ ';
    }
    elseif ($s->current_streak >= 7) {
      $fire = '🔥 ';
    }

    $output .= '<div class="medremind-streak-row">';
    $output .= '<span>' . check_plain($s->name) . '</span>';
    $output .= '<span>' . $fire . $s->current_streak . ' days</span>';
    $output .= '<span>🏆 ' . $s->best_streak . ' days</span>';
    $output .= '</div>';
  }

  if (!$has_streaks) {
    $output .= '<div class="medremind-streak-row"><span>No streak data yet.</span><span></span><span></span></div>';
  }

  $output .= '</div>';
  $output .= '</div>';

  return $output;
}