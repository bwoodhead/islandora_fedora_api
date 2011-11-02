<?php
/**
 * @file 
 * Fedora API Exceptions
 */

/**
 * Exception thrown when an Object isn't found in the repository.
 */
class FedoraAPIObjectNotFoundException extends Exception {

  /**
   * The PID of the Object not found.
   *
   * @var string
   */
  public $pid;

  /**
   * Construct an Object Not Found exception.
   *
   * @param string $pid
   *   The pid of the item not found.
   * @param string $message
   *   (Optional) The message to be reported. Defaults to: "Not Found".
   * @param int $code
   *   (Optional) Defaults to 404.
   */
  function __construct($pid, $message = 'Not Found.', $code = 404) {
    $this->pid = $pid;
    parent::__construct($message);
  }

}

?>