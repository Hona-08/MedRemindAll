<?php  ?>
<div class="medremind-dashboard">
  <!-- User Bar -->
  <div class="medremind-user-bar">
    <span class="user-greeting">👋 Hello, <strong><?php print check_plain($user_name); ?></strong></span>
    <div class="user-bar-actions">
      <a href="<?php print url('medremind/profile'); ?>" class="user-bar-link">👤 Profile</a>
      <a href="<?php print url('medremind/logout'); ?>" class="user-bar-link user-bar-logout">Logout</a>
    </div>
  </div>

  <!-- Stats Bar -->
  <div class="medremind-stats-bar">
    <div class="stat-card stat-total"><div class="stat-number"><?php print (int)$stats['total']; ?></div><div class="stat-label"><?php print t('Total Meds'); ?></div></div>
    <div class="stat-card stat-taken"><div class="stat-number"><?php print (int)$stats['taken']; ?></div><div class="stat-label"><?php print t('Taken Today'); ?></div></div>
    <div class="stat-card stat-missed"><div class="stat-number"><?php print (int)$stats['missed']; ?></div><div class="stat-label"><?php print t('Missed'); ?></div></div>
    <div class="stat-card stat-streak"><div class="stat-number"><?php print (int)$stats['streak']; ?></div><div class="stat-label"><?php print t('Day Streak'); ?></div></div>
  </div>

  <div class="dashboard-actions">
    <?php print l(t('+ Add Medication'), 'medremind/add', array('attributes' => array('class' => array('medremind-add-btn')))); ?>
  </div>

  <div class="medremind-section">
    <h3><?php print t("Today's Medications"); ?> <span class="section-date"><?php print format_date(REQUEST_TIME, 'custom', 'l, F j'); ?></span></h3>

    <?php if (!empty($medications)): ?>
      <div class="medremind-med-list">
        <?php foreach ($medications as $med): ?>
          <div class="med-card <?php if (!empty($med->taken_today)) print 'med-card-taken'; ?>" data-med-id="<?php print (int)$med->med_id; ?>">
            <div class="med-card-info">
              <div class="med-card-icon"><?php print !empty($med->taken_today) ? '✅' : '💊'; ?></div>
              <div class="med-card-details">
                <h4 class="med-name"><?php print check_plain($med->name); ?></h4>
                <p class="med-dosage"><?php print check_plain($med->dosage); ?> &bull; <?php print check_plain($med->frequency); ?></p>
                <?php if (!empty($med->notes)): ?><p class="med-notes">📝 <?php print check_plain($med->notes); ?></p><?php endif; ?>
              </div>
            </div>
            <div class="med-card-time">
              <div class="next-dose-label"><?php print t('Next'); ?></div>
              <div class="next-dose-time"><?php print $med->next_dose; ?></div>
            </div>
            <div class="med-card-actions">
              <?php if (empty($med->taken_today)): ?>
                <a href="<?php print url('medremind/take/' . $med->med_id . '/ajax'); ?>" class="med-btn med-btn-take" data-action="take" data-med-id="<?php print (int)$med->med_id; ?>">✅ <?php print t('Take'); ?></a>
                <a href="<?php print url('medremind/skip/' . $med->med_id . '/ajax'); ?>" class="med-btn med-btn-skip" data-action="skip" data-med-id="<?php print (int)$med->med_id; ?>">⏭️ <?php print t('Skip'); ?></a>
              <?php else: ?>
                <span class="med-taken-badge">✅ <?php print t('Taken'); ?></span>
              <?php endif; ?>
            </div>
            <?php if (!empty($med->streak) && $med->streak > 0): ?>
              <div class="med-card-streak">🔥 <?php print (int)$med->streak; ?> <?php print t('day streak'); ?></div>
            <?php endif; ?>
            <div class="med-card-manage">
              <?php print l(t('Edit'), 'medremind/edit/' . $med->med_id, array('attributes' => array('class' => array('med-link-edit')))); ?>
              | <?php print l(t('Delete'), 'medremind/delete/' . $med->med_id, array('attributes' => array('class' => array('med-link-delete')))); ?>
            </div>
          </div>
        <?php endforeach; ?>
      </div>
    <?php else: ?>
      <div class="medremind-empty">
        <div class="empty-icon">💊</div>
        <h4><?php print t('No medications yet'); ?></h4>
        <p><?php print t('Start by adding your first medication.'); ?></p>
        <?php print l(t('+ Add Medication'), 'medremind/add', array('attributes' => array('class' => array('medremind-add-btn')))); ?>
      </div>
    <?php endif; ?>
  </div>
</div>