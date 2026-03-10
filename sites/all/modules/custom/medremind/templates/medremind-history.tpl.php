<?php
/**
 * @file
 * Medication history page template.
 * Shows dose log, adherence percentage, and streak info.
 *
 * Available variables:
 *   $logs       - Array of dose log entries
 *   $month      - Current month being viewed
 *   $adherence  - Adherence percentage (0-100)
 */
?>

<div class="medremind-history">

  <!-- Adherence Score -->
  <div class="adherence-card">
    <div class="adherence-circle">
      <div class="adherence-number"><?php print $adherence; ?>%</div>
    </div>
    <div class="adherence-label"><?php print t('Adherence Rate'); ?></div>
    <div class="adherence-month"><?php print check_plain($month); ?></div>
  </div>

  <!-- History Log -->
  <div class="medremind-section">
    <h3><?php print t('Dose History'); ?></h3>

    <?php if (!empty($logs)): ?>

      <div class="history-list">
        <?php foreach ($logs as $log): ?>

          <div class="history-item history-<?php print check_plain($log->action); ?>">
            <!-- ↑ Adds class like history-taken, history-missed, history-skipped
                 Each gets different color via CSS -->

            <div class="history-icon">
              <?php
                // Show different icon based on action
                switch ($log->action) {
                  case 'taken':
                    print '✅';
                    break;
                  case 'missed':
                    print '❌';
                    break;
                  case 'skipped':
                    print '⏭️';
                    break;
                }
              ?>
            </div>

            <div class="history-details">
              <strong><?php print check_plain($log->med_name); ?></strong>
              <span class="history-action">
                <?php print check_plain($log->action); ?>
              </span>
            </div>

            <div class="history-time">
              <?php print format_date($log->action_time, 'short'); ?>
              <!-- ↑ format_date() = Drupal's date formatter
                   Converts timestamp to readable date
                   'short' = format defined in admin settings -->
            </div>

          </div>

        <?php endforeach; ?>
      </div>

    <?php else: ?>

      <div class="medremind-empty">
        <p><?php print t('No dose history recorded yet.'); ?></p>
      </div>

    <?php endif; ?>

  </div>
<?php
      // Pagination
      $pager = theme('pager');
      if ($pager):
    ?>
      <div class="medremind-pager"><?php print $pager; ?></div>
    <?php endif; ?>
</div>