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
 * A low level wrapper on fedora APIA functions
 */
class FedoraAPIA {

  /**
   * The FedoraConnection object for this FedoraAPI.
   *
   * @var FedoraConnection
   */
  public $connection;

  /**
   * Constructor for a new FedoraAPI object.
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
   * Query fedora to return a list of objects.
   *
   * @param string $terms
   *   (optional) A phrase represented as a sequence of characters (including the ? and * wildcards) for the search.
   *   If this sequence is found in any of the fields for an object, the object is considered a match. Do NOT use
   *   this parameter in combination with the "query" parameter.
   * @param string $query
   *   (optional) A sequence of space-separated conditions. A condition consists of a metadata element name followed
   *   directly by an operator, followed directly by a value. Valid element names are (pid, label, state, ownerId,
   *   cDate, mDate, dcmDate, title, creator, subject, description, publisher, contributor, date, type, format,
   *   identifier, source, language, relation, coverage, rights). Valid operators are: contains (), equals (=),
   *   greater than (>), less than (<), greater than or equals (>=), less than or equals (<=). The contains () operator
   *   may be used in combination with the ? and * wildcards to query for simple string patterns. Space-separators
   *   should be encoded in the URL as %20. Operators must be encoded when used in the URL syntax as follows: the (=)
   *   operator must be encoded as %3D, the (>) operator as %3E, the (<) operator as %3C, the (>=) operator
   *   as %3E%3D, the (<=) operator as %3C%3D, and the (~) operator as %7E. Values may be any string. If the string
   *   contains a space, the value should begin and end with a single quote character ('). If all conditions are met
   *   for an object, the object is considered a match. Do NOT use this parameter in combination with the
   *   "terms" parameter.
   * @param string,int $maxResults
   *   (optional) Default: 25. The maximum number of results that the server should provide at once.
   * @param string $resultFormat
   *   (optional) Default: XML. The preferred output format. Options: HTML, XML.
   * @param array $displayFields
   *   (optional) Default: array('pid', 'title'). The fields to be returned.
   *
   * @throws FedoraAPIRestException
   *
   * @return StdClass HTTP Response object. 'data' has XML set of results
   * @code
   *   <?xml version="1.0" encoding="UTF-8"?>
   *   <result xmlns="http://www.fedora.info/definitions/1/0/types/">
   *     <resultList>
   *     <objectFields>
   *       <pid>islandora:collectionCModel</pid>
   *       <title>Islandora Collection Content Model</title>
   *     </objectFields>
   *     <objectFields>
   *       <pid>islandora:strict_pdf</pid>
   *       <title>Strict PDF</title>
   *     </objectFields>
   *     </resultList>
   *   </result>
   * @endcode
   */
  public function findObjects($terms = '', $query = '', $maxResults = '', $resultFormat = 'xml', $displayFields = array('pid', 'title')
  ) {
    $fedora_url = $this->connection->requestURL();
    $request = "$fedora_url/objects?";

    if (!empty($terms)) {
      $request .= "terms=" . drupal_encode_path($terms);
    }
    else if (!empty($query)) {
      $request .= "query=" . drupal_encode_path($query);
    }

    $request .= "&resultFormat=$resultFormat";

    foreach ($displayFields as $displayField) {
      $request .= "&$displayField=true";
    }
    $response = drupal_http_request($request);

    if (!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * Get the default dissemination of a datastream. (Get the contents of the datastream.)
   *
   * @param String $pid
   *   Persistent identifier of the digital object.
   * @param String $dsID
   *   Datastream identifier.
   * @param String $asOfDateTime
   *   (optional) Default: NULL. Indicates that the result should be relative to the digital object as it existed
   *   at the given date and time.
   * @param String $download
   *   (optional) Default: FALSE. If true, a content-disposition header value "attachment" will be included in the
   *   response, prompting the user to save the datastream as a file.  A content-disposition header value "inline"
   *   will be used otherwise.  The filename used in the header is generated by examining in order: RELS-INT for the
   *   relationship fedora-model:downloadFilename, the datastream label, and the datastream ID.  The file extension
   *   (apart from where the filename is specified in RELS-INT) is determined from the MIMETYPE.  The order in which
   *   these filename sources are searched, and whether or not to generate an extension from the MIMETYPE, is
   *   configured in fedora.fcfg.  The file used to map between MIMETYPEs and extensions is mime-to-extensions.xml
   *   located in the server config directory.
   *
   * @return StdClass HTTP Response object. The direct response from fedora.
   */
  public function getDatastreamDissemination($pid, $dsID, $asOfDateTime = NULL, $download = NULL) {
    $pid = drupal_encode_path($pid);
    $fedora_url = $this->connection->requestURL();
    $request = "$fedora_url/objects/$pid/datastreams/$dsID/content";
    $request .= ( !empty($asOfDateTime) ? "&asOfDateTime=$asOfDateTime" : '');
    if (!empty($download)) {
      $request .= ( !empty($asOfDateTime) ? '&' : '?');
      $request .= "download=$download";
    }

    $response = drupal_http_request($request);

    if (!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * Get a datastream dissemination from Fedora.
   *
   * @param String $pid
   *   persistent identifier of the digital object
   * @param String $sdefPid
   *   persistent identifier of the sDef defining the methods
   * @param String $method
   *   method to invoke
   * @param String $methodParameters
   *   A key-value paired array of parameters required by the method
   *
   * @return StdClass HTTP response object from fedora
   */
  public function getDissemination($pid, $sdefPid, $method, $methodParameters = array()) {
    $pid = drupal_encode_path($pid);
    $sdefPid = drupal_encode_path($sdefPid);

    $fedora_url = $this->connection->requestURL();
    $request = "$fedora_url/objects/$pid/methods/$sdefPid/$method?";

    //$request .= (!empty($asOfDateTime) ? "&asOfDateTime=$asOfDateTime" : '');
    foreach ($methodParameters as $param_name => $param_value) {
      $request .= $param_name . (!empty($param_value) ? "=$param_value&" : '&');
    }

    $response = drupal_http_request($request);

    if (!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * Get the change history for the datastream.
   *
   * @param String $pid
   *   Persistent identifier of the digital object.
   * @param String $format
   *   (Optional) Default: XML. The preferred output format (xml, html).
   *
   * @return HTTP response object.
   * @code
   *   $response->data looks like:
   *   <?xml version="1.0" encoding="utf-16"?>
   *   <fedoraObjectHistory xsi:schemaLocation="http://www.fedora.info/definitions/1/0/access/
   *     http://localhost:8080/fedoraObjectHistory.xsd" pid="demo:29" xmlns:xsd="http://www.w3.org/2001/XMLSchema"
   *     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
   *       <objectChangeDate>2008-07-02T05:09:43.234Z</objectChangeDate>
   *   </fedoraObjectHistory>
   *  @endcode
   */
  public function getObjectHistory($pid, $format = 'xml') {
    $pid = drupal_encode_path($pid);
    $fedora_url = $this->connection->requestURL();

    $request = "$fedora_url/objects/$pid/versions?format=$format";
    $response = drupal_http_request($request);

    if (!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * Implements the getObjectProfile Fedora API-A method.
   *
   * @param String $pid
   *   Persistent identifier of the digital object.
   * @param String $format
   *   (Optional) Default: XML. The preferred output format. Options: HTML, XML.
   * @param String $asOfDateTime
   *   (Optional) Indicates that the result should be relative to the digital object as it existed on the given date.
   *   Date Format: yyyy-MM-dd or yyyy-MM-ddTHH:mm:ssZ
   *
   * @return StdClass HTTP Response object. 'data' has XML response string
   * @code
   *   <?xml version="1.0" encoding="utf-16"?>
   *   <objectProfile xsi:schemaLocation="http://www.fedora.info/definitions/1/0/access/
   *   http://localhost:8080/objectProfile.xsd" pid="islandora:demos"
   *   xmlns:xsd="http://www.w3.org/2001/XMLSchema"
   *   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
   *     <objLabel>Islandora Demo Collections</objLabel>
   *     <objOwnerId>fedoraAdmin</objOwnerId>
   *     <objModels>
   *       <model>info:fedora/islandora:collectionCModel</model>
   *       <model>info:fedora/fedora-system:FedoraObject-3.0</model>
   *     </objModels>
   *     <objCreateDate>2009-03-10T07:09:53.333Z</objCreateDate>
   *     <objLastModDate>2010-03-20T23:39:58.490Z</objLastModDate>
   *     <objDissIndexViewURL>
   *         http://localhost:8080/fedora/get/islandora:demos/fedora-system:3/viewMethodIndex
   *     </objDissIndexViewURL>
   *     <objItemIndexViewURL>
   *         http://localhost:8080/fedora/get/islandora:demos/fedora-system:3/viewItemIndex
   *     </objItemIndexViewURL>
   *     <objState>A</objState>
   *   </objectProfile>
   * @endcode
   */
  public function getObjectProfile($pid, $format = 'xml', $asOfDateTime = '') {
    $pid = drupal_encode_path($pid);
    $fedora_url = $this->connection->requestURL();

    $request = "$fedora_url/objects/$pid?format=$format";
    $request .= ( !empty($asOfDateTime) ? "&asOfDateTime=$asOfDateTime" : '');
    $response = drupal_http_request($request);

    if (!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * List all the datastreams that are associated with this PID.
   *
   * @param <type> $pid
   *   Persistent identifier of the digital object.
   * @param <type> $format
   *   (optional) Default: XML. The preferred output format. Options: XML, HTML.
   * @param <type> $asOfDateTime
   *   (optional) Indicates that the result should be relative to the digital object as it existed on the given date.
   *   Date Format: yyyy-MM-dd or yyyy-MM-ddTHH:mm:ssZ
   *
   * @return StdClass HTTP Response object. 'data' has XML Response
   * @code
   *   <?xml version="1.0" encoding="utf-16"?>
   *   <objectDatastreams xsi:schemaLocation="http://www.fedora.info/definitions/1/0/access/
   *     http://localhost:8080/listDatastreams.xsd" pid="islandora:demos" baseURL="http://localhost:8080/fedora/"
   *     xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
   *     <datastream dsid="DC" label="Dublin Core Record for this object" mimeType="text/xml" />
   *     <datastream dsid="RELS-EXT" label="RDF Statements about this object" mimeType="application/rdf+xml" />
   *     <datastream dsid="COLLECTION_POLICY" label="Collection Policy" mimeType="text/xml" />
   *     <datastream dsid="TN" label="Thumbnail.png" mimeType="image/png" />
   *   </objectDatastreams>
   * @endcode
   */
  public function listDatastreams($pid, $format = 'xml', $asOfDateTime = '') {
    $pid = drupal_encode_path($pid);
    $fedora_url = $this->connection->requestURL();

    $request = "$fedora_url/objects/$pid/datastreams?format=$format";
    $request .= ( !empty($asOfDateTime) ? "&asOfDateTime=$asOfDateTime" : '');
    $response = drupal_http_request($request);

    if (!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

  /**
   * Implements the listMethods Fedora API-A method.
   *
   * @param String $pid
   *   Persistent identifier of the digital object.
   * @param String $sdefPid
   *   (Optional) Persistent identifier of the SDef defining the methods.
   * @param String $format
   *   (Optional) Default: XML. The preferred output format. Options: XML, HTML.
   * @param String $asOfDateTime
   *   (Optional) Indicates that the result should be relative to the digital object as it existed on the given date.
   *   Date Format: yyyy-MM-dd or yyyy-MM-ddTHH:mm:ssZ
   *
   * @return StdClass HTTP Response object. 'data' has response XML.
   * @code
   *   <?xml version="1.0" encoding="utf-16"?>
   *   <objectMethods xsi:schemaLocation="http://www.fedora.info/definitions/1/0/access/
   *   http://localhost:8080/listMethods.xsd" pid="islandora:demos" baseURL="http://localhost:8080/fedora/"
   *   xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
   *     <sDef pid="fedora-system:3">
   *         <method name="viewObjectProfile"></method>
   *         <method name="viewMethodIndex"></method>
   *         <method name="viewItemIndex"></method>
   *         <method name="viewDublinCore"></method>
   *     </sDef>
   *   </objectMethods>
   * @endcode
   */
  public function listMethods($pid, $sdefPid = '', $format = 'xml', $asOfDateTime = '') {
    $pid = drupal_encode_path($pid);
    $fedora_url = $this->connection->requestURL();

    $request = "$fedora_url/objects/$pid/methods";
    $request .= ( !empty($sdefPid) ? "/$sdefPid" : '');
    $request .= "?format=$format";
    $request .= ( !empty($asOfDateTime) ? "&asOfDateTime=$asOfDateTime" : '');
    $response = drupal_http_request($request);

    if (!FedoraAPIUtils::testSuccess($response)) {
      throw new FedoraApiRestException($response);
    }

    return $response;
  }

}

?>