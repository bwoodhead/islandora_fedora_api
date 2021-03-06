<?php

/**
 * @file
 * A wrapper for Fedora's raw API calls to present Fedora items as manageable objects.
 */

/**
 * Class presenting the Fedora Object in a nice OO fashion.
 */
class FedoraObject extends PropertyObject {

  /**
   * The PID of the fedora object represented by the instance of this class.
   *
   * @var string
   */
  public $pid = NULL;

  /**
   * The FedoraClient Object used by this class.
   *
   * @var FedoraClient object
   */
  public $client = NULL;

  private $objectProfile = NULL;
  private $_datastreams = NULL;
  private static $instantiated_pids = array();

  /**
   * Constructs an object to represent an item in the Fedora repository.
   *
   * @param string $pid
   *   The PID of the Fedora Item to construct an object for.
   * @param FedoraConnection $connection
   *   (optional) The FedoraConnection object to use to connect. If not provided one will be created with the
   *   default parameters by calling new FedoraConnection().
   * @throws FedoraAPIObjectNotFoundException
   * @throws FedoraAPIRestException
   */
  function __construct($pid, $client = NULL) {
    if ($client) {
      $this->client = $client;
    }
    else {
      module_load_include('repository.inc', 'islandora_fedora_api');
      
      // Make a default client using Drupal's variables.
      $this->client = new FedoraRepository();
    }
    $this->pid = $pid;
    if (isset(FedoraItem::$instantiated_pids[$pid])) {
      $this->client = & FedoraItem::$instantiated_pids[$pid]->client;
      $this->objectProfile = & FedoraItem::$instantiated_pids[$pid]->objectProfile;
      $this->_datastreams = & FedoraItem::$instantiated_pids[$pid]->_datastreams;
      $datastreams_doc = & FedoraItem::$instantiated_pids[$pid]->datastreams_list;
    }
    else {
      // Test if this object exists here, so requests for objects that don't exist
      // don't hit the Fedora logs.
      if(!$this->client->objectExists($pid))
        throw new FedoraAPIObjectNotFoundException($pid);
      $this->_refreshObjectProfile();
      FedoraItem::$instantiated_pids[$pid] = &$this;
    }
  }

  /**
   * Set the objects state to deleted.
   *
   * @throws FedoraAPIRestException
   */
  public function delete() {
    $this->state = 'D';
  }

  /**
   * Purge this object.
   *
   * @throws FedoraAPIRestException
   */
  public function purge() {
    $this->client->api->purgeObject($this->pid);
  }

  /**
   * Add a new datastream.  Wrapper for the addDatastream Fedora API-M method.
   * @param type $dsID
   * @param string $file_path
   * @param string $ds_string
   * @param type $params 
   */
  public function addDatastream($dsID, $file_path = NULL, $ds_string = NULL, $params = array('altIDs' => NULL, 'dsLabel' => NULL, 'dsState' => NULL, 'formatURI' => NULL,
    'checksumType' => NULL, 'checksum' => NULL, 'mimeType' => NULL,
    'logMessage' => NULL)) {

    $response = $this->client->api->addDatastream($this->pid, $dsID, $file_path, $ds_string, $params);
    if ($response->code != 201) {
      throw new FedoraRestException($response->error);
    }
    $this->_refreshDatastreams();
    return $this->datastreams[$dsID];
  }

  /**
   * Returns an associative array of this object's datastreams. Results look like this:
   *
   *  'DC' =>
   *    array
   *      'label' => string 'Dublin Core Record for this object' (length=34)
   *      'MIMEType' => string 'text/xml' (length=8)
   *  'RELS-EXT' =>
   *    array
   *      'label' => string 'RDF Statements about this object' (length=32)
   *      'MIMEType' => string 'application/rdf+xml' (length=19)
   *
   * @return array
   */
  function get_datastreams() {

    if ($this->_datastreams != NULL) {
      return $this->_datastreams;
    }

    $result = $this->client->api->listDatastreams($this->pid);
    if ($result->code != 200) {
      throw new FedoraRestException($result->error);
    }
    $datastreams_doc = new SimpleXMLElement($result->data);
    $ds_list = array();
    if ($datastreams_doc != FALSE) {

      foreach ($datastreams_doc->datastream as $ds) {
        foreach ($ds->attributes() as $field => $value) {
          switch ($field) {
            case 'dsid':
              $dsid = (string) $value;
              break;
            case 'label':
              $dslabel = (string) $value;
              break;
            case 'mimeType':
              $dsmime = (string) $value;
              break;
          }
        }

        if (!empty($dsid) && !empty($dsmime) && $dslabel !== NULL) {
          $ds_list[$dsid] = new FedoraDatastream($this, $dsid, $dslabel, $dsmime);
        }
      }
    }

    $this->_datastreams = $ds_list;
    return $ds_list;
  }

  protected function get_label() {
    return (string) $this->objectProfile->objLabel;
  }

  protected function set_label($label) {
    $this->modifyObject($label);
  }

  protected function get_ownerId() {
    return (string) $this->objectProfile->objOwnerId;
  }

  protected function set_ownerId($ownerId) {
    $this->modifyObject($this->label, $ownerId);
  }

  protected function get_state() {
    return (string) $this->objectProfile->objState;
  }

  protected function set_state($state) {
    $this->modifyObject(NULL, NULL, $state);
  }

  protected function get_pid() {
    return $this->pid;
  }

  protected function get_createDate() {
    return new DateTime((string) $this->objectProfile->objCreateDate);
  }

  protected function get_lastModDate() {
    return new DateTime((string) $this->objectProfile->objLastModDate);
  }

  protected function get_models() {
    $models = array();
    foreach ($this->objectProfile->objModels->model as $model) {
      $models[] = (string) $model;
    }
    return $models;
  }

  private function _refreshDatastreams() {
    $this->_datastreams = NULL;
  }

  private function _refreshObjectProfile() {
    $response = $this->client->api->getObjectProfile($this->pid);

    if ($response->code == 200) {
      $this->objectProfile = new SimpleXMLElement($response->data);
      $this->_datastreams = $this->get_datastreams();
    }
    else {
      throw new FedoraAPIRestException($response);
    }
  }

  /**
   * Wrapper for Fedora's modifyObject REST call. Updates the local objectProfile after each call.
   * 
   * @param type $label
   * @param type $ownerId
   * @param type $state One of 'A' for active, 'D' for deleted, or 'I' for inactive 
   * @param type $logMessage 
   */
  public function modifyObject($label = NULL, $ownerId = NULL, $state = NULL, $logMessage = NULL) {
    $response = $this->client->api->modifyObject($this->pid, $label, $ownerId, $state, $logMessage);
    if ($response->code == 200) {
      $this->_refreshObjectProfile();
      return TRUE;
    }
    else {
      return FALSE;
    }
  }

  static function createItem() {
    $this->client->api->ingest();
  }

}
