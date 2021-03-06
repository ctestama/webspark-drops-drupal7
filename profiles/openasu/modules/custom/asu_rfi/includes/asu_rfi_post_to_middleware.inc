<?php /** @noinspection PhpUndefinedClassInspection */

/**
 * POSTs form data to Salesforce.
 * Used by long form and the multiform, for both UNDERGRAD and GRADUATE submissions.
 * Done one submission at a time.
 * @param array $data - form data to submit
 * @param string $type - Undergrad or graduate
 * @throws EntityMetadataWrapperException
 */
function asu_rfi_post_to_middleware($data, $type) {

  global $base_url;
  $submission_data = array();
  $college_code = $url = $body = '';
  $sf_multi_pending = FALSE;

  ////////////////////////////
  // Get and update RFI submission node
  ////////////////////////////

  $degreeMappings = _asu_rfi_get_degree_field_mappings();
  $nid = $data['submission_nodeid'];
  $node = node_load($nid);
  $nodeArr = array(
    $node->field_rfi_program[LANGUAGE_NONE][0]['value'] => $node->field_rfi_program_name[LANGUAGE_NONE][0]['value']
  );
  $availableCampuses = _asu_rfi_get_campus_location_options($nodeArr);
  $node_wrapper = entity_metadata_wrapper('node', $node);

  // Get information for logging
  $nid_info = (isset($node_wrapper->nid)) ? $node_wrapper->nid->value() : '';

  // Incomplete multiform submissions logic
  if (isset($node_wrapper->field_rfi_submission_status)) {
    if ($node_wrapper->field_rfi_submission_status->value() === "Form_Multi_Partial") {
      // Do not send to SalesForce yet because this is NOT a cron run.
      // This is the initial form submission.
      $sf_multi_pending = TRUE;
      $node_wrapper->field_rfi_submission_status->set('SF_Multi_Pending');
      if (isset($node_wrapper->field_rfi_error_message)) {
        $node_wrapper->field_rfi_error_message->set('Pending to be submitted (regardless of completion state).');
      }
      $node_wrapper->save();
    } elseif ($node_wrapper->field_rfi_submission_status->value() === "SF_Multi_Pending") {
      if (isset($node_wrapper->created) && isset($node_wrapper->field_rfi_error_message)) {
        $created = $node_wrapper->created->value();
        $message = _asu_rfi_create_expired_message($created);
        $node_wrapper->field_rfi_error_message->set($message);
        $node_wrapper->save();
      }
    }
  }

  ////////////////////////////////////////////////
  // Middleware submission check and post creation
  ////////////////////////////////////////////////

  // Send to any middleware?
  $form_mode = variable_get('asu_rfi_form_mode', 'test');
  $middleware_env = variable_get('asu_rfi_middleware_env', 'prod');
  $middleware_access = variable_get('asu_rfi_enterprise_access_', 0);

  // If YES below, start building middleware submission and submit it
  if (($middleware_access === 1 || $middleware_env === 'prod')
    && ($form_mode !== 'none')
    && (!$sf_multi_pending)) {

    $prog_code = $data['program_code'];
    $campus = asu_libs_get_campus_codes($data['campus']);
    $phone = preg_replace('[\D]', '', $data['phone']);
    $mobile = (isset($data['mobile_check']))
      ? str_replace('-', '', $data['mobile_check'])
      : ''; // mobile phone
    $dob = (isset($data['birthdate'])) ? $data['birthdate'] : ''; // date of birth
    $military = (isset($data['military'])) ? $data['military'] : '';  // Military
    $comment = (isset($data['questions'])) ? $data['questions'] : ''; // Comment
    $location = (isset($data['location'])) ? $data['location'] : ''; // Location
    // Online campus check
    if ($location === "no_preference" || $type === ASU_RFI_PROSPECT_LEVEL_GRAD) {
      if (!isset($campus) && count($availableCampuses) == 1
        && (ucfirst($availableCampuses[0]) == "Online")) {
        $location = "Online";
      }
    }
    $address = (isset($data['address'])) ? $data['address'] : ''; // Address
    $city = (isset($data['city'])) ? $data['city'] : ''; // City
    $state = (isset($data['state'])) ? $data['state'] : ''; // State
    $country = (isset($data['country'])) ? $data['country'] : ''; // country

    // GA enterpriseclientid for use in EdPlus Leads submissions.
    $enterpriseclientid = (isset($data['enterpriseclientid'])) ? $data['enterpriseclientid'] : '';
    // Get authentication key
    $mw_auth_key = variable_get('asu_rfi_form_auth_key');

    // Get server diagnostic data for submissions
    $server = _asu_rfi_get_tech_submission_data($data, $type);

    // Submission customization - UNDERGRAD vs GRAD
    if ($type === ASU_RFI_PROSPECT_LEVEL_UNDERGRAD) {

      $hashkey = sha1($mw_auth_key . '+' . $nid);
      // Select proper ENV for Salesforce GRAD POSTs.
      $tld = ($middleware_env === 'prod') ? ASU_RFI_MIDDLEWARE_UNDERGRAD_PROD : ASU_RFI_MIDDLEWARE_UNDERGRAD_QA;
      // Default submission URL (non-Online)
      $url = $tld . "/asu_rfi_dedupe/submit_rfi/$hashkey/$nid";

      // Get college code from database based on program code
      if ($prog_code == "" || empty($prog_code)) {
        $prog_code = "Exploratory";
        $college_code = null;
      } else {
        $college_code = _asu_rfi_get_college_code($degreeMappings, $prog_code);
      }

      // Compile DEFAULT Undergrad RFI submission data array
      $submission_data = array(
        'source_id' => $server['source'],
        'base_url' => $server['domain'],
        'prod_test_flag' => $form_mode,
        'first_name' => $data['first_name'],
        'last_name' => $data['last_name'],
        'prospect_level' => ASU_RFI_PROSPECT_LEVEL_UNDERGRAD,
        'student_type' => $data['student_type'],
        'program' => $prog_code,
        'starting_semester' => $data['start_date'], ////// Fix me @TODO 7208
        'email' => $data['email'],
        'zip' => $data['zipcode'],
        'phone' => $phone,
        'birth_date' => $dob,
        'country' => $country,
        'sms' => $mobile,
        'comments' => $comment,
        'street' => $address,
        'city' => $city,
        'state' => $state,
        'intnl_student' => $data['citizenship'],
        'veteran_status' => $military,
        'campus' => $campus,
        'location' => $location,
      );

      // Use JSON for ALL UNDERGRAD submissions
      $body = json_encode($submission_data);
    }

    if ($type === ASU_RFI_PROSPECT_LEVEL_GRAD) {
      // Semester term name
      $term = (isset($data['semester_name'])) ? $data['semester_name'] : '';
      // If NEITHER the USA NOR Canada, drop the address fields' data.
      if ($country !== "United States" && $country !== "Canada") {
        $address = '';
        $state = '';
        $city = '';
      }

      // Select proper ENV for Salesforce GRAD POSTs.
      $tld = ($middleware_env === 'prod') ? ASU_RFI_MIDDLEWARE_GRAD_PROD : ASU_RFI_MIDDLEWARE_GRAD_QA;
      $url = $tld . '/prospect_form_post';

      // Build DEFAULT Graduate data array for Salesforce
      $submission_data = array(
        'source' => $server['source'],
        'firstName' => $data['first_name'],
        'lastName' => $data['last_name'],
        'emailAddress' => $data['email'],
        'phoneNumber' => $phone,
        'projectedEnrollment' => $term,
        'addressLine1' => $address,
        'city' => $city,
        'state' => $state,
        'countryOfResidence' => $country,
        'birthdate' => $dob,
        'militaryStatus' => $military,
        'questions' => $comment,
        'poiCode' => $prog_code,
        'zip' => $data['zipcode'],
        'iStudent' => $data['citizenship'],
        'smsNumber' => $mobile,
        'campusSelection' => $campus,
      );
      $body = $submission_data; // No JSON encoding for default, non-online GRAD submissions
    }

    // Alter Online campus submission data
    if (strtolower($location) === "online") {

      // ALL Online campus RFI submissions are supposed to go to webforms middleware - not to requestinfo.asu.edu.
      $tld = ($middleware_env === 'prod') ? $url = ASU_RFI_MIDDLEWARE_UNDERGRAD_PROD : ASU_RFI_MIDDLEWARE_UNDERGRAD_QA;

      if ($type === ASU_RFI_PROSPECT_LEVEL_GRAD) {
        // Match different "Online" campus spelling
        $submission_data['campusSelection'] = "ONLNE";
      }
      $asuOnlineList = _asu_rfi_parse_cached_programs();
      $managedOnline = _asu_rfi_check_managed_online($prog_code, $asuOnlineList);

      if ($managedOnline == TRUE) {
        if ($type === ASU_RFI_PROSPECT_LEVEL_GRAD) {
          $college_code = _asu_rfi_get_college_code($degreeMappings, $prog_code);
        }
        $submission_data = _asu_rfi_online_fields_alter($type, $submission_data, $server, $college_code, $enterpriseclientid, $nid, $form_mode);
        // Use JSON for Online GRADUATE submissions
        $body = json_encode($submission_data);
        $fieldHash = sha1($mw_auth_key . '+' . $body);
        $url = $tld . "/asu_edplus_middleware/submit/$fieldHash";
      }
    } else {
      // Remove meaningless data points for GRAD submissions
      if ($type === ASU_RFI_PROSPECT_LEVEL_GRAD) {
        $grad_rebuild = FALSE;
        foreach ($submission_data as $key_value => $result_array) {
          if (($result_array == "None")) {
            unset($submission_data[$key_value]);
            $grad_rebuild = TRUE;
          }
        }
        // Rebuild default, no JSON for default GRAD submissions if necessary.
        if ($grad_rebuild === TRUE) {
          $body = $submission_data;
        }
      }
    }

    ///////////////////////////////////
    // POST data to SalesForce middleware via cURL if the form is ready to submit.
    // NOTE: Some middleware servers (test_mode and/or ENV URLs) are not available
    // outside the ASU network (specifically Pantheon sites).
    ///////////////////////////////////

    try {
      $curl = curl_init($url);
      curl_setopt($curl, CURLOPT_RETURNTRANSFER, TRUE); // If you don't want to use any of the return information, set to fa$
      curl_setopt($curl, CURLOPT_CUSTOMREQUEST, 'POST');
      curl_setopt($curl, CURLOPT_POSTFIELDS, $body); // data mapping
      curl_setopt($curl, CURLOPT_SSLVERSION, 1); // This will set the security protocol to TLSv1
      curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, false);
      $response = curl_exec($curl);
      $curl_info = curl_getinfo($curl); // Used by your drupal messages below

      variable_set('asu_rfi_last_attempt_SF_posting', time());

      // Get the response from Salesforce middleware
      // $message = (strpos($response, "-1 ") === 0) ? explode(':', $response) : '';

      // SUCCESSFUL POST
      if (($curl_info['http_code'] === 200)) {
        $curl_message = 'SUCCESS - Submission ' . $nid_info . ' posted to middleware. Will be deleted by the next cron run.';
        if (isset($node_wrapper->field_rfi_submission_status)) {
          $node_wrapper->field_rfi_submission_status->set('SF_Completed');
        }
        if (isset($node_wrapper->field_rfi_error_message)) {
          $node_wrapper->field_rfi_error_message->set($curl_message);
        }
        $node_wrapper->save();
        // Record time of latest successful post
        variable_set('asu_rfi_last_success_SF_posting', time());
        flogger('asu_rfi', $curl_message, array(), 8, TRUE, TRUE, TRUE);
      } // FAILED POST
      else {
        /** @noinspection PhpUndefinedFieldInspection */
        $created = $node_wrapper->created->value();
        $expired_message = _asu_rfi_create_expired_message($created);
        $curl_message = 'FAILED - Submission #' . $nid_info . ' to __' . $url . '__. '
          . 'HTTP code ' . $curl_info['http_code']
          . ': "' . $response . '" ' . "\n" . $expired_message;

        if (($curl_info['http_code'] >= 400)) {
          // update error message
          // Get admin email address from admin settings page
          $to_email = variable_get('asu_rfi_error_email', NULL);
          // Send error email to admins.
          if (!empty($to_email) && !empty($mw_auth_key)) { // Admin email and middleware key are locally set
            if (!isset($data['cron_run']) || ($data['cron_run'] != "Cron_running")) { // Not a cron job
              _asu_rfi_create_failed_post_email($to_email, $nid);
            }
          }
        }
        // Save the error message in the submission node
        if (isset($node_wrapper->field_rfi_submission_status)) {
          $node_wrapper->field_rfi_submission_status->set('SF_Failed');
        }
        if (isset($node_wrapper->field_rfi_error_message)) {
          $node_wrapper->field_rfi_error_message->set($curl_message);
        }
        $node_wrapper->save();
        // Log/print messaging to end user and set timestamp
        flogger('asu_rfi', $curl_message, array(), WATCHDOG_ERROR, FALSE, FALSE, TRUE);
        variable_set('asu_rfi_last_failure_SF_posting', time());
      }

      curl_close($curl);

      if ($response === FALSE) {
        throw new Exception(curl_error($curl), curl_errno($curl));
      }
    } catch (Exception $e) {
      $msg_start = 'POST to ' . $base_url . ' failed. HTTP CODE: '
        . ((isset($curl_info)) ? $curl_info['http_code'] : 'unknown') . '. ';
      $message_final = $msg_start . (((int)$e->getCode() <= 0) ? ' May be due to
    middleware access restrictions.' : '');
      flogger('asu_rfi', $message_final, array(), WATCHDOG_ERROR);
      trigger_error(sprintf(
        'Curl failed with error #%d: %s' . $message_final,
        $e->getCode(), $e->getMessage()),
        E_USER_ERROR);
    }
  }

  else { // Do NOT attempt submission to SF middleware
    if ($sf_multi_pending) { // Multiform submission not ready to submit yet
      $message_final = 'WAITING - MF submission will be submitted 1) when the student completes
      the form, or 2) it is resubmitted by either a cron job or a manual submission.';
    } elseif ($form_mode === 'none') {
      $message_final = 'DISABLED - Submission #' . $nid_info . ' never sent to middleware. (RFI Test Mode = none).';
    } else { // Other connectivity issue?
      $message_final = 'CANCELLED - Submission #' . $nid_info . ' cannot be sent to middleware. (Site may not be able to reach QA'
      . ' middleware).';
    }

    // Update submission node
    if (isset($node_wrapper->field_rfi_error_message)) {
      $node_wrapper->field_rfi_error_message->set($message_final);
    }
    if (isset($node_wrapper->field_rfi_submission_status)) {
      $node_wrapper->field_rfi_submission_status->set('SF_Failed');
    }
    $node_wrapper->save();

    flogger('asu_rfi', $message_final, array(), WATCHDOG_NOTICE, TRUE, FALSE, TRUE);
  }
}
