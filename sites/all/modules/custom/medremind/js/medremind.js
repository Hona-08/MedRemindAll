/**
 * @file MedRemind JavaScript — AJAX + Toast + Sound.
 */
(function ($) {

  // ============================================
  // SOUND — Fix browser autoplay policy
  // ============================================
  var audioCtx = null;
  var soundReady = false;

  // Initialize audio on FIRST user click anywhere on page
  function _initSound() {
    if (soundReady) return;
    try {
      audioCtx = new (window.AudioContext || window.webkitAudioContext)();
      // Resume if suspended (browser policy)
      if (audioCtx.state === 'suspended') {
        audioCtx.resume();
      }
      soundReady = true;
    } catch (e) {
      soundReady = false;
    }
  }

  // Play a beep sound
  function _playSound(freq, duration) {
    if (!soundReady || !audioCtx) return;
    try {
      // Resume context every time (some browsers re-suspend)
      if (audioCtx.state === 'suspended') {
        audioCtx.resume();
      }
      var osc = audioCtx.createOscillator();
      var gain = audioCtx.createGain();
      osc.connect(gain);
      gain.connect(audioCtx.destination);
      osc.frequency.value = freq || 880;
      osc.type = 'sine';
      gain.gain.setValueAtTime(0.3, audioCtx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.01, audioCtx.currentTime + (duration || 0.4));
      osc.start(audioCtx.currentTime);
      osc.stop(audioCtx.currentTime + (duration || 0.4));
    } catch (e) {}
  }

  // Enable sound on first click/touch anywhere
  $(document).one('click touchstart', function () {
    _initSound();
  });


  // ============================================
  // TOAST SYSTEM
  // ============================================
  function _ensureToastContainer() {
    if ($('#medremind-toast-container').length === 0) {
      $('body').append('<div id="medremind-toast-container"></div>');
    }
  }

  function _showToast(icon, title, message, type, duration) {
    _ensureToastContainer();
    type = type || 'success';
    duration = duration || 5000;

    var id = 'toast-' + Date.now();

    var html = '<div class="medremind-toast medremind-toast-' + type + '" id="' + id + '">'
      + '<div class="toast-icon">' + icon + '</div>'
      + '<div class="toast-content">'
      + '<div class="toast-title">' + title + '</div>'
      + '<div class="toast-message">' + message + '</div>'
      + '</div>'
      + '<button class="toast-close">&times;</button>'
      + '</div>';

    var $toast = $(html);
    $('#medremind-toast-container').prepend($toast);

    // Slide in
    setTimeout(function () {
      $toast.addClass('toast-visible');
    }, 10);

    // Play sound
    _playSound(880, 0.3);

    // Auto dismiss
    if (duration > 0) {
      setTimeout(function () {
        _dismissToast(id);
      }, duration);
    }

    // Close button
    $toast.find('.toast-close').click(function () {
      _dismissToast(id);
    });
  }

  function _dismissToast(id) {
    var $toast = $('#' + id);
    $toast.removeClass('toast-visible').addClass('toast-hiding');
    setTimeout(function () {
      $toast.remove();
    }, 400);
  }


  // ============================================
  // MAIN BEHAVIOR
  // ============================================
  Drupal.behaviors.medremind = {
    attach: function (context) {

      // ----- Take button -----
      $('.med-btn-take', context).once('mr-take', function () {
        $(this).click(function (e) {
          e.preventDefault();

          // Init sound on this click (first interaction)
          _initSound();

          var $btn = $(this);
          var $card = $btn.closest('.med-card');

          // Prevent double-click
          if ($btn.hasClass('disabled')) return;
          $btn.addClass('disabled').text('Taking...');

          $.getJSON($btn.attr('href'), function (r) {
            if (r.status === 'ok') {
              // Update card UI
              $card.addClass('med-card-taken');
              $card.find('.med-card-actions')
                .html('<span class="med-taken-badge">✅ Taken</span>');
              $card.find('.med-card-icon').text('✅');

              // Update streak
              if (r.streak > 0) {
                $card.find('.med-card-streak').remove();
                $card.find('.med-card-manage').before(
                  '<div class="med-card-streak">🔥 ' + r.streak + ' day streak</div>'
                );
              }

              // Update stat counter
              var $num = $('.stat-taken .stat-number');
              var count = parseInt($num.text()) || 0;
              $num.text(count + 1);

              // Toast + Sound
              _showToast('✅', r.med_name + ' taken!', 'Great job! Keep it up.', 'success', 5000);

              // Streak milestone toast
              var milestones = [7, 14, 21, 30, 60, 90, 100];
              if (r.streak && milestones.indexOf(r.streak) !== -1) {
                setTimeout(function () {
                  _playSound(1200, 0.5);
                  _showToast('🔥', r.streak + '-day streak!',
                    'Amazing! You\'re on fire with ' + r.med_name + '!', 'streak', 7000);
                }, 1500);
              }
            }
          }).fail(function () {
            $btn.removeClass('disabled').text('✅ Take');
            _showToast('❌', 'Error', 'Could not record dose. Try again.', 'error', 5000);
          });
        });
      });

      // ----- Skip button -----
      $('.med-btn-skip', context).once('mr-skip', function () {
        $(this).click(function (e) {
          e.preventDefault();
          if (!confirm('Skip this dose?')) return;

          _initSound();

          var $btn = $(this);
          var $card = $btn.closest('.med-card');

          if ($btn.hasClass('disabled')) return;
          $btn.addClass('disabled').text('Skipping...');

          $.getJSON($btn.attr('href'), function (r) {
            if (r.status === 'ok') {
              $card.find('.med-card-actions')
                .html('<span class="med-taken-badge" style="background:#fff3e0;color:#e67e22;">⏭️ Skipped</span>');
              $card.find('.med-card-streak').remove();

              _showToast('⏭️', r.med_name + ' skipped',
                'Try not to miss the next one!', 'warning', 5000);
            }
          }).fail(function () {
            $btn.removeClass('disabled').text('⏭️ Skip');
            _showToast('❌', 'Error', 'Could not skip dose. Try again.', 'error', 5000);
          });
        });
      });

    }
  };

})(jQuery);