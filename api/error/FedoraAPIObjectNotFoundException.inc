<?php

/**
 * @file 
 * Fedora API Exceptions
 */

/**
 * General exception thrown on unexpected REST response.
 */
class FedoraAPIRestException extends Exception {

  /**
   * This contains the unexpected REST response from drupal_http_request.
   *
   * @var Drupal StdClass HTTP Response object.
   */
  public $http;

  /**
   * The PID of the Object that was being manipulated. Defaults to NULL if there was no PID involved.
   *
   * @var string
   */
  public $pid;

  /**
   * Constructs a new Rest Exception. $http->error is passed to parent as $message and 
   * $http->code is passed to $code.
   *
   * @param StdClass HTTP Response $http
   *   The HTTP response from drupal_http_request.
   * @param string $pid
   *   (Optional) The PID that the error occured on.
   */
  function __construct($http, $pid = NULL) {
    $this->http = $http;
    $this->pid = $pid;
    parent::__construct($http->error, $http->code);
  }
}

?>
