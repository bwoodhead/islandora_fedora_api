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
 * A low level wrapper on fedora APIM functions
 */
class FedoraAPIM {
  /**
   * The FedoraConnection object for this FedoraAPIM.
   *
   * @var FedoraConnection
   */
  public $connection;

  /**
   * Constructor for a new FedoraAPIM object.
   *
   * @param FedoraConnection $connection
   *   (optional) a new one will be created if not supplied.
   */
  function __construct($connection = NULL) {
    if ($connection != NULL) {
      $this->connection = $connection;
    }
    else {
      // Construct a connection from Drupal's API settings.
      $this->connection = new FedoraConnection();
    }
  }


  /**
   * Add a new datastream to a fedora object. The datastreams are sent to Fedora using a multipart post if a string or
   * file is provided. Otherwise the dsLocation field is used. $file_path, $ds_string and $params['dsLocation'] are
   * all optional, however ONE and only ONE must be specified for this function.
   *
   * @param string $pid
   *   Persistent identifier of the digital object.
   * @param string $dsID
   *   Datastream identifier.
   * @param <type> $file_path
   *   (Optional)
   * @param <type> $ds_string
   *   (Optional)
   * @param <type> $params
   *   (Optional)
   *
   * @return <type>
   *
   * @todo Do more sophisticated MIME type detection. Possibly including retrieving headers of dsLocation.
   * @todo Figure out the multipart mimetype function here. I think it is buggy and should be using a dynamically
   *   generated boundry. I also think that we should be passing strings through the multipart mimetype functionalty.
   *   Much of this code was taken from here: http://drupal.org/node/270997.
   *   There is some good RFC documentation on this here:
   *     http://tools.ietf.org/html/rfc2388
   *     http://tools.ietf.org/html/rfc2046#section-5
   *     http://tools.ietf.org/html/rfc2045#page-14
   *     http://www.w3.org/TR/REC-html40/interact/forms.html#h-17.13.4.2
   * @todo also sort out the commented code in this function
   */
  public function addDatastream(
    $pid, $dsID, $file_path = NULL, $ds_string = NULL,
    $params = array(
      'controlGroup' => NULL,
      'dsLocation' => NULL,
      'altIDs' => NULL,
      'dsLabel' => NULL,
      'dsState' => NULL,
      'formatURI' => NULL,
      'checksumType' => NULL,
      'checksum' => NULL,
      'mimeType' => NULL,
      'logMessage' => NULL,
    )
  ) {
    $pid = drupal_encode_path($pid);

    foreach (array('dsLabel', 'formatURI', 'logMessage') as $string_param) {
      if (key_exists($string_param, $params) && $params[$string_param] != NULL) {
        $params[$string_param] = drupal_encode_path($params[$string_param]);
      }
    }

    $request_params = NULL;

    $headers = array();

    if (!empty($params['dsLocation'])) {
      if ($params['controlGroup'] == 'X') {
        $headers['Content-Type'] = 'text/xml';
      }
      elseif (!empty($params['mimeType'])) {
        $headers['Content-Type'] = $params['mimeType'];
      }
      else {
        // TODO: More sophisticated MIME type detection. Possibly including retrieving headers of dsLocation
        $headers['Content-Type'] = 'application/x-octet-stream';
      }
    }
    elseif (!empty($file_path)) {
      // We must construct a multipart HTTP POST.
      $boundary = 'A0sFSD';
      $headers["Content-Type"] = "multipart/form-data; boundary=$boundary";
      $request_params = FedoraAPIUtils::multipartEncode($boundary, array('file' => $file_path));
    }
    elseif (!empty($ds_string)) {
      // We must construct a multipart HTTP POST.
      $boundary = 'A0sFSD';
      $headers["Content-Type"] = "text/plain";
      $request_params = $ds_string;
      //$request_params = _fedora_api_multipart_encode($boundary, array('string' => $ds_string));
    }
    else {
      return NULL;
    }

    $fedora_url = $this->connection->requestURL();
    $request = $fedora_url . "/objects/$pid/datastreams/$dsID?";

    foreach ($params as $param_name => $param_value) {
      $request .= $param_value != NULL ? "$param_name=$param_value&" : '';
    }

    $response = drupal_http_request($request, array('headers' => $headers, 'method' => 'POST', 'data' => $request_params));

    if(!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }


    return $response;
  }

  /**
   * Export a Fedora object with the given PID.
   *
   * @param String $pid
   *   Persistent identifier of the digital object.
   * @param String $format
   *   (Optional) The XML format to export. Options: info:fedora/fedora-system:FOXML-1.1 (default),
   *   info:fedora/fedora-system:FOXML-1.0, info:fedora/fedora-system:METSFedoraExt-1.1,
   *   info:fedora/fedora-system:METSFedoraExt-1.0, info:fedora/fedora-system:ATOM-1.1,
   *   info:fedora/fedora-system:ATOMZip-1.1
   * @param String $context
   *   (Optional) The export context, which determines how datastream URLs and content are represented.
   *   Options: public (default), migrate, archive
   * @param String $encoding
   *   (Optional) The preferred encoding of the exported XML.
   * @return StdClass HTTP Response object. The 'data' element contains the requested export data.
   */
  public function export(
    $pid,
    $format = 'info:fedora/fedora-system:FOXML-1.1',
    $context = 'public', $encoding = 'UTF-8'
  ) {
    $pid = drupal_encode_path($pid);
    $format = drupal_encode_path($format);

    $fedora_url = $this->connection->requestURL();
    $request = $fedora_url . "/objects/$pid/export?";

    $request .= (!empty($format) ? "format=$format&" : '')
             . (!empty($context) ? "context=$context&" : '')
             . (!empty($encoding) ? "encoding=$encoding" : '');
    $response = drupal_http_request($request);

    if(!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

 /**
  * Returns the XML metadata information about a datastream.
  *
  * @param String $pid
  *   Persistent identifier of the digital object.
  * @param String $dsID
  *   Datastream identifier.
  * @param String $format
  *   (optional) Default: xml. The preferred output format. Options: xml or html.
  * @param String $asOfDateTime
  *   (optional) Indicates that the result should be relative to the digital object as it existed on the given date.
  *   Format: yyyy-MM-dd or yyyy-MM-ddTHH:mm:ssZ
  * @param String $validateChecksum
  *   (optional) verifies that the Datastream content has not changed since the checksum was initially computed. If
  *   asOfDateTime is null, Fedora will use the most recent version.
  *
  * @return StdClass HTTP Response object. 'data' has XML set of results.
  * @code
  *   <?xml version="1.0" encoding="UTF-8"?>
  *   <datastreamProfile xmlns="http://www.fedora.info/definitions/1/0/management/"
  *     xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  *     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  *     xsi:schemaLocation="http://www.fedora.info/definitions/1/0/management/
  *     http://www.fedora.info/definitions/1/0/datastreamProfile.xsd"
  *     pid="demo:2" dsID="DC">
  *   <dsLabel>Dublin Core Record</dsLabel>
  *   <dsVersionID>DC.0</dsVersionID>
  *   <dsCreateDate>2011-05-05T13:23:58.858Z</dsCreateDate>
  *   <dsState>A</dsState>
  *   <dsMIME>text/xml</dsMIME>
  *   <dsFormatURI/>
  *   <dsControlGroup>X</dsControlGroup>
  *   <dsSize>753</dsSize>
  *   <dsVersionable>true</dsVersionable>
  *   <dsInfoType/>
  *   <dsLocation>demo:2+DC+DC.0</dsLocation>
  *   <dsLocationType/>
  *   <dsChecksumType>DISABLED</dsChecksumType>
  *   <dsChecksum>none</dsChecksum>
  *   </datastreamProfile>
  * @endcode
  */
  public function getDatastream($pid, $dsID, $format = 'xml', $asOfDateTime = NULL, $validateChecksum = FALSE) {
    $pid = drupal_encode_path($pid);
    $fedora_url = $this->connection->requestURL();
    $request = "$fedora_url/objects/$pid/datastreams/$dsID";
    $format = strtolower($format);
    $separator = '?';
    if (in_array($format, array('html', 'xml'))) {
      $request .= "{$separator}format=$format";
      $separator = '&';
    }
    if (!empty($asOfDateTime)) {
      $request .= "{$separator}asOfDateTime=$asOfDateTime";
      $separator = '&';
    }
    if (!empty($validateChecksum)) {
      $request .= "{$separator}validateChecksum=$validateChecksum";
    }
    $response = drupal_http_request($request);

    if(!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * Get a new PID.
   *
   * @param int $numPIDS
   *   (Optional) Default: 1.
   * @param string $namespace
   *   (Optional) Default: ''
   * @param string $format
   *   (Optional) Options: XML (default), HTML
   * @return StdClass HTTP response
   * @code
   * <?xml version="1.0" encoding="UTF-8"?>
   * <pidList  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
   *   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
   *   xsi:schemaLocation="http://www.fedora.info/definitions/1/0/management/
   *   http://localhost:8080/getNextPIDInfo.xsd">
   *   <pid>changeme:2</pid>
   * </pidList>
   * @endcode
   *
   * @todo clean up the reqaut code to make it more readable.
   * @todo should we make the namespace default more sensible, something like islandora?
   */
  public function getNextPID($numPIDS = 1, $namespace = '', $format = 'xml') {
    $fedora_url = $this->connection->requestURL();
    $request = $fedora_url . "/objects/nextPID?";
    $request .= (!empty($numPIDS) ? "numPIDS=$numPIDS&" : '')
             . (!empty($namespace) ? "namespace=$namespace&" : '')
             . (!empty($format) ? "format=$format" : '');
    $response = drupal_http_request($request, array('headers' => array(), 'method' => 'POST'));

    if(!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * Get the fedora objects XML.
   *
   * @param String $pid
   * @return StdClass HTTP response object.
   */
  public function getObjectXML($pid) {
    $pid = drupal_encode_path($pid);
    $fedora_url = $this->connection->requestURL();
    $request = $fedora_url . "/objects/$pid/objectXML";
    $response = drupal_http_request($request);

    if(!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * Ingest a FOXML item into Fedora. This could be a string, a file, or a reference to a file that Fedora will
   * go and retreve.
   *
   * @param String $foxml_file
   * @param String $foxml_string
   * @param String $pid
   * @param String $new
   * @param String $label
   * @param String $format
   * @param String $encoding
   * @param String $namespace
   * @param String $ownerId
   * @param String $logMessage
   * @param String $ignoreMime
   * @return StdClass HTTP Response Object
   * $response->data contains just the PID of the ingested object (plain text).
   *
   * @todo See the notes for addDatastream. I think we should revisit how the REST calls are done
   *   on this function.
   */
  public function ingest( $foxml_file = NULL, $foxml_string = NULL, $pid = NULL, $new = TRUE, $label = NULL,
    $format = 'info:fedora/fedora-system:FOXML-1.1', $encoding = 'UTF-8', $namespace = NULL, $ownerId = NULL,
    $logMessage = NULL, $ignoreMime = 'false'
  ){
    if (empty($pid)) {
      // Set $new to TRUE regardless of what the user specified if no PID is given
      $pid = "new";
    }
    else {
      $pid = drupal_encode_path($pid);
    }
    $label = drupal_encode_path($label);
    $format = drupal_encode_path($format);
    $logMessage = drupal_encode_path($logMessage);
    $params = NULL;
    $headers = array(
      'Keep-Alive' => 115,
      'Connection' => 'keep-alive',
    );
    if (!empty($foxml_file)) {
      // We must construct a multipart HTTP POST.
      $boundary = 'A0sFSD';
      $headers["Content-Type"] = "multipart/form-data; boundary=$boundary";
      $params = FedoraAPIUtils::multipartEncode($boundary, array('foxml_file' => $foxml_file));
    }
    elseif (!empty($foxml_string)) {
      // We must construct a multipart HTTP POST.
      $boundary = 'A0sFSD';
      $headers["Content-Type"] = "multipart/form-data; boundary=$boundary";
      $params = FedoraAPIUtils::multipartEncode($boundary, array('foxml_string' => $foxml_string));
    }

    $fedora_url = $this->connection->requestURL();

    $request = $fedora_url . "/objects/$pid?";

    $request .= (!empty($label) ? "label=$label&" : '')
             . (!empty($format) ? "format=$format&" : '')
             . (!empty($encoding) ? "encoding=$encoding&" : '')
             . (!empty($namespace) ? "namespace=$namespace&" : '')
             . (!empty($ownerId) ? "ownerId=$ownerId&" : '')
             . (!empty($logMessage) ? "logMessage=$logMessage&" : '')
             . (!empty($ignoreMime) ? "ignoreMime=$ignoreMime&" : '');
    $response = drupal_http_request($request, array('headers' => $headers, 'method' => 'POST', 'data' => $params));

    if(!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;

  }

  /**
   * Modify the parameters of a Fedora datastream.
   *
   * Note: Due ot a bug in Fedora you need to specify the Mime Type for each call.
   *
   * @param <type> $pid
   * @param <type> $dsID
   * @param <type> $file_path
   * @param <type> $dsLocation
   * @param <type> $altIDs
   * @param <type> $dsLabel
   * @param <type> $versionable
   * @param <type> $dsState
   * @param <type> $formatURI
   * @param <type> $checksumType
   * @param <type> $checksum
   * @param <type> $mimeType
   * @param <type> $logMessage
   * @param <type> $force
   * @param <type> $ignoreContent
   * @return StdClass HTTP response object
   * @code
   * <?xml version="1.0" encoding="utf-16"?>
   * <datastreamProfile xsi:schemaLocation="http://www.fedora.info/definitions/1/0/management/
   *   http://localhost:8080/datastreamProfile.xsd" pid="islandora:demos" dsID="TN"
   *   xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
   *     <dsLabel>Thumbnail.png</dsLabel>
   *     <dsVersionID>TN.4</dsVersionID>
   *     <dsCreateDate>2010-04-08T12:27:01.966Z</dsCreateDate>
   *     <dsState>A</dsState>
   *     <dsMIME>image/png</dsMIME>
   *     <dsFormatURI/>
   *     <dsControlGroup>M</dsControlGroup>
   *     <dsSize>0</dsSize>
   *     <dsVersionable>true</dsVersionable>
   *     <dsInfoType/>
   *     <dsLocation>islandora:demos+TN+TN.4</dsLocation>
   *     <dsLocationType>INTERNAL_ID</dsLocationType>
   *     <dsChecksumType>DISABLED</dsChecksumType>
   *     <dsChecksum>none</dsChecksum>
   * </datastreamProfile>
   * @endcode
   *
   * @todo Verify the claim above about mimetypes. Perhaps reference a Fedora issue numebr.
   * @todo Finish the PHPDoc for this function.
   * @todo Cleanup the commented out code specifying the headers.
   */
  public function modifyDatastream( $pid, $dsID, $file_path = NULL, $ds_string = NULL,
    $params = array(
      'dsLocation' => NULL,
      'altIDs' => NULL,
      'dsLabel' => NULL,
      'versionable' => NULL,
      'dsState' => NULL,
      'formatURI' => NULL,
      'checksumType' => NULL,
      'checksum' => NULL,
      'mimeType' => NULL,
      'logMessage' => NULL,
      'force' => NULL,
      'ignoreContent' => NULL
    )
  ){
    $pid = drupal_encode_path($pid);

    foreach (array('dsLabel', 'formatURI', 'logMessage') as $string_param) {
      if (key_exists($string_param, $params) && $params[$string_param] != NULL) {
        $params[$string_param] = drupal_encode_path($params[$string_param]);
      }
    }

    $request_params = NULL;
    $headers = array(
   //   'Keep-Alive' => 115,
   //   'Connection' => 'keep-alive',
    );

    $fedora_url = $this->connection->requestURL();

    $request = $fedora_url . "/objects/$pid/datastreams/$dsID?";

    foreach ($params as $param_name => $param_value) {
      $request .= $param_value != NULL ? "$param_name=$param_value&" : '';
    }

    $data = (!empty($file_path) ? file_get_contents($file_path) : $ds_string);
    $response = drupal_http_request($request, array('headers' => $headers, 'method' => 'PUT', 'data' => $data));

    if(!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * @todo Please document this function.
   */
  public function modifyObject($pid, $label = NULL, $ownerId = NULL, $state = NULL, $logMessage = NULL) {
    $pid = drupal_encode_path($pid);
    $label = drupal_encode_path($label);
    $logMessage = drupal_encode_path($logMessage);
    $fedora_url = $this->connection->requestURL();
    $request = $fedora_url . "/objects/$pid?";
    $request .= (!empty($label) ? "label=$label&" : '')
             . (!empty($ownerId) ? "ownerId=$ownerId&" : '')
             . (!empty($state) ? "state=$state" : '')
             . (!empty($logMessage) ? "logMessage=$logMessage" : '');
    $response = drupal_http_request($request, array('headers' => array(), 'method' => 'PUT'));

    if(!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * Purge a datastream, deleting its contents and all its revisions.
   *
   * @param String $pid
   *   Persistent identifier of the digital object.
   * @param String $dsID
   *   Datastream identifier.
   * @param String $startDT
   *   (Optional) Format: yyyy-MM-dd or yyyy-MM-ddTHH:mm:ssZ - the (inclusive) start date-time stamp of the range.
   *   If not specified, this is taken to be the lowest possible value, and thus, the entire version history up to
   *   the endDT will be purged.
   * @param String $endDT
   *   (Optional) Format: yyyy-MM-dd or yyyy-MM-ddTHH:mm:ssZ - the (inclusive) ending date-time stamp of the range. If
   *   not specified, this is taken to be the greatest possible value, and thus, the entire version history back to the
   *   startDT will be purged.
   * @param String $logMessage
   *   (Optional) A message describing the activity being performed.
   * @param String $force
   *   (Optional) Options: true, false (default). Force the update even if it would break a data contract.
   * @return StdClass HTTP response object
   *   On Success $response->code will be 204 and $response->error will be 'No content'.
   */
  public function purgeDatastream($pid, $dsID, $startDT = '', $endDT = '', $logMessage = '', $force = 'false') {
    $pid = drupal_encode_path($pid);
    $logMessage = drupal_encode_path($logMessage);
    $fedora_url = $this->connection->requestURL();
    $request = $fedora_url . "/objects/$pid/datastreams/$dsID?";
    $request .= (!empty($startDT) ? "startDT=$startDT&" : '')
             . (!empty($endDT) ? "endDT=$endDT&" : '')
             . (!empty($logMessage) ? "logMessage=$logMessage" : '')
             . (!empty($force) ? "force=$force" : '');
    $response = drupal_http_request($request, array('headers' => array(), 'method' => 'DELETE'));

    if(!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * Purge an object.
   *
   * @param String $pid
   *   Persistent identifier of the digital object.
   * @param String $logMessage
   *   (Optional) A message describing the activity being performed.
   * @param String $force
   *   (Optional) Options: true, false (default). Force the update even if it would break a data contract.
   * @return StdClass HTTP Response object
   * $response->code is 204 on successful delete, $response->error is "No content".
   */
  public function purgeObject($pid, $logMessage = '', $force = 'false') {
    $pid = drupal_encode_path($pid);
    $logMessage = drupal_encode_path($logMessage);
    $fedora_url = $this->connection->requestURL();
    $request = $fedora_url . "/objects/$pid?";
    $request .= (!empty($logMessage) ? "logMessage=$logMessage" : '')
             . (!empty($force) ? "force=$force" : '');
    $response = drupal_http_request($request, array('headers' => array(), 'method' => 'DELETE'));

    if(!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

}

?>a