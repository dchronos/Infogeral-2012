<?php

// $Id$

/**
 * @file
 * Provides token services
 */


/**
 * Administration form for FTP settings
 * @return array, drupal form array
 *
 */
function mm_ftp_admin_token_form() {
  $form = array();
  $form['mm_ftp_admin'] = array(
    '#type' => 'markup',
    '#title' => t('FTP Tokens'),
    '#collapsible' => TRUE,
    '#value' => t('FTP tokens are items that an end user adds to a file name and the uploads the file to your FTP server. The token associates the file with that specific user.'),
  );
  $form['mm_ftp_admin']['mm_ftp_token_duration'] = array(
    '#title' => t('Token duration'),
    '#type' => 'textfield',
    '#default_value' => variable_get('mm_ftp_token_duration', 2880),
    '#description' => t('The token duration is time in minutes that a users token is valid for uploading. This associates a user with the content that they uploaded on the ftp server.'),
  );
  $text = t("You can upload files via our FTP server with the following information. \nAppend the on to the end of your file name. We will associate your user account with your file through this token.\n".
    "Our FTP server is located at:\n ftp://myserver.com \nYou can use the user name anonymous and password anonymous\n".
    "Your file should look like\n  myfile.mov.32a18b");
  $form['mm_ftp_admin']['mm_ftp_info'] = array(
    '#title' => t('FTP information'),
    '#type' => 'textarea',
    '#default_value' => variable_get('mm_ftp_info', $text),
    '#description' => t('This text is displayed on your !token_page. It will also be
        mailed to the user when they request a token. You can use the following tokens
        in your text to be replaced by the values: [user name], [token], [site name], [site url].',
      array('!token_page' => l(t('token page'), 'ftp_token'))
    ),
  );
  $text = t("Hi [user name], \nYou can upload files via our FTP server with the following information. \n
    Append this token: [token] on to the end of your file name. We will associate your user account
    with your file through this token.\n
    Our FTP server is located at:\n ftp://myserver.com \n
    You can use the user name anonymous and password anonymous\n
    Your file should look like\n  myfile.mov.[token] \n Thanks! [site name] [site url]");
  $form['mm_ftp_admin']['mm_ftp_email_text'] = array(
    '#title' => t('Email explanation'),
    '#type' => 'textarea',
    '#default_value' => variable_get('mm_ftp_email_text', $text),
    '#description' => t('This text will be mailed to the user when they request a token. You can use the following tokens in your text to be replaced by the values: [user name], [token], [site name], [site url].'),
  );
  return system_settings_form($form);
}


/**
 * build the token creation form
 * @return array
 *   drupal form array
 */
function mm_ftp_token_ftp_form($form_state) {
  global $user;
  $form['mmftp_info']['text'] = array(
    '#type' => 'markup',
    '#value' => check_markup(variable_get('mm_ftp_info', '')),
  );
  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $user->uid,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Get token'),
  );
  return $form;
}


/**
 * handles the form submit, enters token into the db
 * @param string $form_id is the form id
 * @param array $form_values is a form array
 */
function mm_ftp_token_ftp_form_submit($form, &$form_state) {
  global $user;

  // create a token
  $token = drupal_substr(drupal_get_token(time()), 0, MM_FTP_TOKEN_LENGTH);

  // save the token
  db_query("INSERT INTO {mm_ftp_tokens} (uid, token, date) VALUES (%d, '%s', '%s')", $user->uid, $token, time() );

  // email the user the token
  drupal_mail('mm_ftp', 'ftp_token', $user->mail, user_preferred_language($account),
    array(
      'account' => $user,
      'token' => $token
    )
  );

  drupal_set_message(
    t('We have created the token @token for you. We will also email you your token. You are free to start uploading.',
      array('@token' => $token)
    )
  );
  return;
}



/**
 * Checkes a token string against the database to see if it is a user id.
 * This will modify the $filepath if there is a token on it
 *
 * @param $file
 *   array, file data
 * @return boolean, string
 *
 */
function mm_ftp_token_check(&$file) {
  // Did we get a token?
  if (mm_ftp_token_extract($file)) {
    if ($uid = db_result(db_query('SELECT uid FROM {mm_ftp_tokens} WHERE token = "%s"', $file->token))) {
      $file['uid'] = $uid;
      return TRUE;
    }
  }
}


/**
 * Extracts a token from the end of a file name
 *
 * @param $file
 *   array, file data
 * @return boolean
 */
function mm_ftp_token_extract(&$file) {
  // Build a pattern to look for the token code at the end
  // of the string
  $pattern = '/\.([0-9a-zA-z]{' . MM_FTP_TOKEN_LENGTH . '})$/';
  if (preg_match($pattern, $filename, $matches)) {
    $file->token = $matches[1];
    return TRUE;
  }
  return FALSE;
}