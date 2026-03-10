<?php
/**
 * @file
 * Custom login, registration, and user pages for MedRemind.
 */


/**
 * Custom login page callback: /medremind/login
 */
function medremind_login_page() {
  global $user;

  // Already logged in? Redirect to dashboard
  if ($user->uid > 0) {
    drupal_goto('medremind');
  }

  $output = '<div class="medremind-auth-page">';
  $output .= '<div class="medremind-auth-card">';

  // Logo / Header
  $output .= '<div class="medremind-auth-header">';
  $output .= '<div class="medremind-auth-logo">💊</div>';
  $output .= '<h1>MedRemind</h1>';
  $output .= '<p>Track your medications, never miss a dose.</p>';
  $output .= '</div>';

  // Login form
  $form = drupal_get_form('medremind_login_form');
  $output .= drupal_render($form);

  // Links
  $output .= '<div class="medremind-auth-links">';
  $output .= '<a href="' . url('medremind/register') . '">Don\'t have an account? <strong>Sign Up</strong></a>';
  $output .= '<a href="' . url('user/password') . '">Forgot password?</a>';
  $output .= '</div>';

  $output .= '</div>';
  $output .= '</div>';

  return $output;
}


/**
 * Custom login form.
 */
function medremind_login_form($form, &$form_state) {
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Username or Email'),
    '#required' => TRUE,
    '#maxlength' => 60,
    '#attributes' => array('placeholder' => t('Enter your username'), 'autofocus' => 'autofocus'),
  );

  $form['pass'] = array(
    '#type' => 'password',
    '#title' => t('Password'),
    '#required' => TRUE,
    '#attributes' => array('placeholder' => t('Enter your password')),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Sign In'),
    '#attributes' => array('class' => array('medremind-auth-btn')),
  );

  return $form;
}


/**
 * Login form validation.
 */
function medremind_login_form_validate($form, &$form_state) {
  $name = trim($form_state['values']['name']);
  $pass = $form_state['values']['pass'];

  if (empty($name)) {
    form_set_error('name', t('Please enter your username.'));
    return;
  }

  if (empty($pass)) {
    form_set_error('pass', t('Please enter your password.'));
    return;
  }

  // Check if input is email — find username
  if (valid_email_address($name)) {
    $account = user_load_by_mail($name);
    if ($account) {
      $name = $account->name;
    }
  }

  // Authenticate
  $uid = user_authenticate($name, $pass);

  if ($uid) {
    $form_state['uid'] = $uid;
  }
  else {
    form_set_error('name', t('Invalid username or password.'));
    watchdog('medremind', 'Login failed for @name.', array('@name' => $name));
  }
}


/**
 * Login form submit.
 */
function medremind_login_form_submit($form, &$form_state) {
  global $user;

  $uid = $form_state['uid'];
  $account = user_load($uid);

  // Log the user in
  $user = $account;
  $login_array = array('name' => $account->name);
  user_login_finalize($login_array);

  drupal_set_message(t('Welcome back, @name!', array('@name' => format_username($account))));

  // Redirect to dashboard
  $form_state['redirect'] = 'medremind';
}


/**
 * Custom registration page callback: /medremind/register
 */
function medremind_register_page() {
  global $user;

  // Already logged in?
  if ($user->uid > 0) {
    drupal_goto('medremind');
  }

  // Check if registration is allowed
  if (!variable_get('medremind_allow_registration', 1)) {
    return '<div class="medremind-auth-page"><div class="medremind-auth-card">'
      . '<div class="medremind-auth-header"><div class="medremind-auth-logo">🔒</div>'
      . '<h1>Registration Closed</h1>'
      . '<p>New registrations are currently disabled.</p></div>'
      . '<div class="medremind-auth-links"><a href="' . url('medremind/login') . '">Back to Login</a></div>'
      . '</div></div>';
  }

  $output = '<div class="medremind-auth-page">';
  $output .= '<div class="medremind-auth-card">';

  // Header
  $output .= '<div class="medremind-auth-header">';
  $output .= '<div class="medremind-auth-logo">💊</div>';
  $output .= '<h1>Create Account</h1>';
  $output .= '<p>Start tracking your medications today.</p>';
  $output .= '</div>';

  // Registration form
  $form = drupal_get_form('medremind_register_form');
  $output .= drupal_render($form);

  // Links
  $output .= '<div class="medremind-auth-links">';
  $output .= '<a href="' . url('medremind/login') . '">Already have an account? <strong>Sign In</strong></a>';
  $output .= '</div>';

  $output .= '</div>';
  $output .= '</div>';

  return $output;
}


/**
 * Custom registration form.
 */
function medremind_register_form($form, &$form_state) {
  $form['username'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#required' => TRUE,
    '#maxlength' => 60,
    '#attributes' => array('placeholder' => t('Choose a username'), 'autofocus' => 'autofocus'),
  );

  $form['mail'] = array(
    '#type' => 'textfield',
    '#title' => t('Email Address'),
    '#required' => TRUE,
    '#maxlength' => 254,
    '#attributes' => array('placeholder' => t('your@email.com')),
  );

  $form['pass1'] = array(
    '#type' => 'password',
    '#title' => t('Password'),
    '#required' => TRUE,
    '#attributes' => array('placeholder' => t('Create a password (min 6 characters)')),
  );

  $form['pass2'] = array(
    '#type' => 'password',
    '#title' => t('Confirm Password'),
    '#required' => TRUE,
    '#attributes' => array('placeholder' => t('Confirm your password')),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create Account'),
    '#attributes' => array('class' => array('medremind-auth-btn')),
  );

  return $form;
}


/**
 * Registration form validation.
 */
function medremind_register_form_validate($form, &$form_state) {
  $v = $form_state['values'];

  $username = trim($v['username']);
  $mail = trim($v['mail']);
  $pass1 = $v['pass1'];
  $pass2 = $v['pass2'];

  // Username validation
  if (drupal_strlen($username) < 3) {
    form_set_error('username', t('Username must be at least 3 characters.'));
  }

  if (preg_match('/[^a-zA-Z0-9_]/', $username)) {
    form_set_error('username', t('Username can only contain letters, numbers, and underscores.'));
  }

  // Check if username exists
  if (user_load_by_name($username)) {
    form_set_error('username', t('This username is already taken.'));
  }

  // Email validation
  if (!valid_email_address($mail)) {
    form_set_error('mail', t('Please enter a valid email address.'));
  }

  // Check if email exists
  if (user_load_by_mail($mail)) {
    form_set_error('mail', t('An account with this email already exists.'));
  }

  // Password validation
  if (drupal_strlen($pass1) < 6) {
    form_set_error('pass1', t('Password must be at least 6 characters.'));
  }

  if ($pass1 !== $pass2) {
    form_set_error('pass2', t('Passwords do not match.'));
  }
}


/**
 * Registration form submit.
 */
function medremind_register_form_submit($form, &$form_state) {
  global $user;

  $v = $form_state['values'];

  // Create the user account
  $new_user = array(
    'name' => trim($v['username']),
    'mail' => trim($v['mail']),
    'pass' => $v['pass1'],
    'status' => 1,  // Active immediately
    'init' => trim($v['mail']),
    'roles' => array(DRUPAL_AUTHENTICATED_RID => 'authenticated user'),
  );

  $account = user_save(NULL, $new_user);

  if ($account) {
    // Grant 'use medremind' permission automatically
    // We add the user to an appropriate role or just let the permission
    // be assigned to 'authenticated user' role

    // Auto-login the new user
    $user = $account;
    $login_array = array('name' => $account->name);
    user_login_finalize($login_array);

    drupal_set_message(t('Welcome to MedRemind, @name! Your account has been created.', array(
      '@name' => format_username($account),
    )));

    // Redirect to dashboard
    $form_state['redirect'] = 'medremind';
  }
  else {
    drupal_set_message(t('There was a problem creating your account. Please try again.'), 'error');
  }
}


/**
 * Custom logout callback: /medremind/logout
 */
function medremind_logout_page() {
  global $user;

  if ($user->uid > 0) {
    watchdog('medremind', 'User @name logged out.', array('@name' => $user->name));
    module_invoke_all('user_logout', $user);

    // Destroy the session
    session_destroy();

    drupal_set_message(t('You have been logged out. See you next time!'));
  }

  drupal_goto('medremind/login');
}


/**
 * User profile page: /medremind/profile
 */
function medremind_profile_page() {
  global $user;

  $account = user_load($user->uid);

  $output = '<div class="medremind-profile-page">';
  $output .= '<h2>👤 My Profile</h2>';

  $output .= '<div class="medremind-profile-card">';

  // Avatar
  $output .= '<div class="medremind-profile-avatar">';
  $first_letter = strtoupper(substr($account->name, 0, 1));
  $output .= '<div class="profile-avatar-circle">' . $first_letter . '</div>';
  $output .= '</div>';

  // User info
  $output .= '<div class="medremind-profile-info">';
  $output .= '<div class="profile-detail"><span class="profile-label">👤 Username:</span> ' . check_plain($account->name) . '</div>';
  $output .= '<div class="profile-detail"><span class="profile-label">📧 Email:</span> ' . check_plain($account->mail) . '</div>';
  $output .= '<div class="profile-detail"><span class="profile-label">📅 Member since:</span> ' . format_date($account->created, 'custom', 'F j, Y') . '</div>';
  $output .= '<div class="profile-detail"><span class="profile-label">🕐 Last login:</span> ' . ($account->login ? format_date($account->login, 'custom', 'F j, Y g:i A') : 'Never') . '</div>';

  // Stats
  $total_meds = 0;
  $total_taken = 0;
  $best_streak = 0;

  if (db_table_exists('medremind_medications')) {
    $total_meds = (int) db_select('medremind_medications', 'm')
      ->condition('m.uid', $user->uid)
      ->countQuery()->execute()->fetchField();
  }
  if (db_table_exists('medremind_log')) {
    $total_taken = (int) db_select('medremind_log', 'l')
      ->condition('l.uid', $user->uid)
      ->condition('l.action', MEDREMIND_ACTION_TAKEN)
      ->countQuery()->execute()->fetchField();
  }
  if (db_table_exists('medremind_streaks')) {
    $best_streak = (int) db_query(
      'SELECT MAX(best_streak) FROM {medremind_streaks} WHERE uid = :uid',
      array(':uid' => $user->uid)
    )->fetchField();
  }

  $output .= '<div class="profile-stats">';
  $output .= '<div class="profile-stat"><span class="profile-stat-num">' . $total_meds . '</span><span class="profile-stat-label">Medications</span></div>';
  $output .= '<div class="profile-stat"><span class="profile-stat-num">' . $total_taken . '</span><span class="profile-stat-label">Doses Taken</span></div>';
  $output .= '<div class="profile-stat"><span class="profile-stat-num">' . $best_streak . '</span><span class="profile-stat-label">Best Streak</span></div>';
  $output .= '</div>';

  $output .= '</div>';

  // Actions
  $output .= '<div class="medremind-profile-actions">';
  $output .= l(t('Edit Profile'), 'user/' . $user->uid . '/edit', array('attributes' => array('class' => array('medremind-btn', 'medremind-btn-primary'))));
  $output .= ' ';
  $output .= l(t('Change Password'), 'user/' . $user->uid . '/edit', array('attributes' => array('class' => array('medremind-btn', 'medremind-btn-outline')), 'fragment' => 'edit-pass'));
  $output .= ' ';
  $output .= l(t('Logout'), 'medremind/logout', array('attributes' => array('class' => array('medremind-btn', 'medremind-btn-danger'))));
  $output .= '</div>';

  $output .= '</div>';
  $output .= '</div>';

  return $output;
}
