<?php

/**
 * @file
 * Lowest-level wrappers for Fedora Commons's REST API functions. 
 * Functions return the HTTP response directly.
 *
 * All of these functions look at the response code of the error and
 * throw FedoraAPIRestException if the response code isn't success (2XX).
 *
 * Much documentation about the API that is wraps can be found here:
 * https://wiki.duraspace.org/display/FCR30/REST+API
 */

/**
 * A collection of helpful utilities for dealing with fedora.
 * 
 * @todo Should this be refactored out into its own file?
 */
class FedoraAPIUtils {

  /**
   * Tests if the given PID fits the general rules for a PID. Must start with a letter, must be less the 64 characters,
   * no special characters, etc. This says nothing about the PID existing, it just tests the string with a regular
   * expression.
   *
   * @param String $pid
   *
   * @return boolean
   *
   * @todo Document where this regex came from. I believe it is in the Fedora documentation somewhere.
   */
  static function validPid($pid) {
    $valid = FALSE;
    if (strlen(trim($pid)) <= 64 && preg_match('/^([A-Za-z0-9]|-|\.)+:(([A-Za-z0-9])|-|\.|~|_|(%[0-9A-F]{2}))+$/', trim($pid))) {
      $valid = TRUE;
    }

    return $valid;
  }

  /**
   * Tests if the given DSID is valid. As above, this function is just a regular expression to validate the DSID, it
   * says nothing about the DSID actually existing.
   *
   * @param string $dsid
   *
   * @return boolean
   *
   * @todo As above we should document where this regex came from. I know I have seen it in the Fedora documenation
   *   somewhere before.
   */
  static function validDsid($dsid) {
    $valid = FALSE;
    if (strlen(trim($dsid)) <= 64 && preg_match('/^[a-zA-Z0-9\_\-\.]+$/', trim($dsid))) {
      $valid = TRUE;
    }

    return $valid;
  }

  /**
   * This function takes a string, and returns a string that is a valid DSID. This is very useful if using filenames
   * as DSIDs as most operating systems file names are less restrictive then DSIDs. This matches the Python
   * implementation of this function.
   * The algorithm is:
   * 1) It removes all characters that are unsupported in a DSID.
   * 2) If the string is too long, it takes the last 64 characters of the string.
   * 3) If the string starts with a number, prepend random letter.
   * 4) If string is empty, make is a random 10 character string.
   *
   * @param String $dsid
   * @return String
   *   The mangled DSID.
   */
  static function mangleDsid($dsid) {
    $new_dsid = trim($dsid);

    $string_array = 'abcdefghijklmnopqustuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';

    $find = '/[^a-zA-Z0-9\.\_\-]/';
    $replace = '';
    $new_dsid = preg_replace($find, $replace, $new_dsid);

    if( strlen($new_dsid) > 64 ) {
      $new_dsid = substr($new_dsid, -64);
    }

    if( preg_match('/^[^a-zA-Z]/', $dsid )) {
      if( strlen($new_dsid) == 64 ) {
        $new_dsid = substr($new_dsid, -63);
      }
      $new_dsid = this::randomString() . $new_dsid;
    }

    if( strlen($new_dsid) == 0 )
      $new_dsid = this::randomString(10);

    return $new_dsid;
  }

  // This is defined for the function above
  private static function randomString($length = 1) {
    $array = 'abcdefghijklmnopqustuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';

    $string = '';

    while( $length > 0 ) {
      $string .= $array[rand(0,51)];
      $length--;
    }

    return $string;
  }


  /**
   * Returns TRUE if the http response code in the supplied $http_response is a 2XX success code.
   *
   * @todo make sure this is correct
   * @todo should we be logging here
   * @param StdObject $http_response
   */
  static function testSuccess($http_response, $log = FALSE, $request = '') {
    $code = (string) $http_response->code;

    if( $code[0] == '2' ) {
      return TRUE;
    }
    else {
      if( $log == TRUE ) {
        this::logError($http_response, $request);
      }
      return FALSE;
    }
  }

  static function logError($http_response, $request) {
    watchdog('fedora api', 'Error executing Fedora REST request %request: %error',
      array('%request' => $request, '%error' => $http_response->error), WATCHDOG_ERROR
    );
  }

  static function multipartEncode($boundary, $params) {
    if (empty($params)) {
      return NULL;
    }

    $output = "";
    foreach ($params as $key => $value) {
      $output .= "--$boundary\r\n";
      if ($key == 'file') {
        $output .= this::multipartEncodeFile($value);
      }
      elseif ($key == 'foxml_file') {
        $output .= this::multipartEncodeXMLFile($value);
      }
      elseif ($key == 'foxml_string') {
        $output .= this::multipartEncodeXMLString($value);
      }
      else {
        $output .= this::multipartEncodeText($key, $value);
      }
    }
    $output .= "--$boundary--";
    return $output;
  }

  protected static function multipartEncodeFile($path) {
    // remove @ at beginning
    if (substr($path, 0, 1) == "@") {
      $path = substr($path, 1);
    }
    $filename = basename($path);
    $mimetype = "application/octet-stream";
    $data = "Content-Disposition: form-data; name=\"file\"; filename=\"$filename\"\r\n";
    $data .= "Content-Transfer-Encoding: binary\r\n";
    $data .= "Content-Type: $mimetype\r\n\r\n";
    $data .= file_get_contents($path) . "\r\n";
    return $data;
  }

  protected static function multipartEncodeXMLFile($path) {
    if (substr($path, 0, 1) == "@") {
      $path = substr($path, 1);
    }
    $filename = basename($path);
    $mimetype = "text/xml";
    $data = "Content-Disposition: form-data; name=\"file\"; filename=\"$filename\"\r\n";
    $data .= "Content-Transfer-Encoding: UTF-8\r\n";
    $data .= "Content-Type: $mimetype\r\n\r\n";
    $data .= file_get_contents($path) . "\r\n";
    return $data;
  }

  protected static function multipartEncodeXMLString($xml_data) {
    $mimetype = "text/xml";
    $data = "Content-Disposition: form-data; name=\"foxml\"; filename=\"FOXML\"\r\n";
    $data .= "Content-Transfer-Encoding: UTF-8\r\n";
    $data .= "Content-Type: $mimetype\r\n\r\n";
    $data .= $xml_data . "\r\n";
    return $data;
  }

  protected static function multipartEncodeText($name, $value) {
    $mimeType = 'text/plain';
    $data = "Content-Disposition: form-data: name=\"$name\" filename=\"$name\"\r\n";
    $data .= "Content-Transfer-Encoding: UTF-8\r\n";
    $data .= "Content-Type: $mimeType\r\n\r\n";
    $data .= "$value\r\n";
    return $data;
    return "Content-Disposition: form-data; name=\"$name\"\r\n\r\n$value\r\n";
  }

}
