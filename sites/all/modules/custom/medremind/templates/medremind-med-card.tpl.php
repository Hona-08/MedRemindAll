<?php
/**
 * @file
 * Single medication card template.
 * Displays one medication with take/skip buttons.
 *
 * Available variables:
 *   $medication  - Medication object with name, dosage, etc.
 *   $next_dose   - Next dose time string
 *   $streak      - Current streak count
 */
?>

<div class="med-card" data-med-id="<?php print $medication->med_id; ?>">
  <!-- ↑ data-med-id = HTML5 data attribute
       JavaScript can read this to know which medication to update
       Used when user clicks Take/Skip buttons -->

  <!-- Left: Medication Info -->
  <div class="med-card-info">

    <div class="med-card-icon">💊</div>

    <div class="med-card-details">
      <h4 class="med-name">
        <?php print check_plain($medication->name); ?>
      </h4>
      <!-- ↑ check_plain() = escapes HTML characters
           Prevents XSS attacks (someone injecting <script> tags)
           ALWAYS use on user-entered data -->

      <p class="med-dosage">
        <?php print check_plain($medication->dosage); ?>
        &bull;
        <?php print check_plain($medication->frequency); ?>
      </p>
      <!-- ↑ &bull; = bullet point character (•) -->

      <?php if (!empty($medication->notes)): ?>
        <p class="med-notes">
          <?php print check_plain($medication->notes); ?>
        </p>
      <?php endif; ?>
    </div>

  </div>

  <!-- Middle: Next Dose Time -->
  <div class="med-card-time">
    <?php if (!empty($next_dose)): ?>
      <div class="next-dose-label"><?php print t('Next Dose'); ?></div>
      <div class="next-dose-time"><?php print check_plain($next_dose); ?></div>
    <?php endif; ?>
  </div>

  <!-- Right: Action Buttons -->
  <div class="med-card-actions">

    <a href="<?php print url('medremind/take/' . $medication->med_id . '/ajax'); ?>"
       class="med-btn med-btn-take"
       data-action="take"
       data-med-id="<?php print $medication->med_id; ?>">
      ✅ <?php print t('Take'); ?>
    </a>
    <!-- ↑ url() = Drupal's URL function, creates proper internal URL
         data-action and data-med-id = for JavaScript AJAX handler -->

    <a href="<?php print url('medremind/skip/' . $medication->med_id . '/ajax'); ?>"
       class="med-btn med-btn-skip"
       data-action="skip"
       data-med-id="<?php print $medication->med_id; ?>">
      ⏭️ <?php print t('Skip'); ?>
    </a>

  </div>

  <!-- Streak Badge -->
  <?php if ($streak > 0): ?>
    <div class="med-card-streak">
      🔥 <?php print $streak; ?> <?php print t('day streak'); ?>
    </div>
  <?php endif; ?>

  <!-- Edit/Delete Links -->
  <div class="med-card-manage">
    <?php print l(t('Edit'), 'medremind/edit/' . $medication->med_id, array(
      'attributes' => array('class' => array('med-link-edit')),
    )); ?>
    |
    <?php print l(t('Delete'), 'medremind/delete/' . $medication->med_id, array(
      'attributes' => array('class' => array('med-link-delete')),
    )); ?>
  </div>

</div>