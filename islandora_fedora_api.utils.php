<?php
/**
 * @file
 *   This file is meant to provide usefull functions for dealing with fedora 
 *   that are not done on individual objects or datastreams
 *   THIS IS PLACEHOLDER DON'T USE IT, 
 *   these are functions we want to re-implement in the new api
 * @todo
 *   remove all calls on islandora core
 * @todo
 *   test
 *   
 * some of these should appear in different files:
 * perhaps we should sublcass object with a collection,
 * and some query things in the repository class
 *   
 */


 /**
 * This function is used to get a list containing all of the islandora collections in a Fedora repo
 * @return $collection_list
 *   an associated array of collection pids and names
 */
function get_all_collections() {
  
  $collection_list=array();
  
  //read in the itql query for getting collections
  $query_file_name=drupal_get_path('module', 'islandora_fedora_api') . '/collection_query.txt';
  $query_file_handle=fopen($query_file_name, "r");
  $query_string=fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);
  //make query
  $collection_list=get_related_objects($query_string);
  //strip out non-applicable collections via namespace
  $collection_list=limit_collections_by_namespace($collection_list);
  return $collection_list;
}

/**
 * This function will reduce the results on a collection search down to those 
 * applicable to this install of Islandora.
 * @author
 *   Paul Pound
 * @author
 *   William Panting
 * @param array $existing_collections
 *   The list of collections before modification
 * @param array $pid_namespaces
 *   The list of namespaces that are applicable to this Islandora install
 * @return array $collections
 *   The collections that exist in the indicated namespaces
 */
function limit_collections_by_namespace($existing_collections, $pid_namespaces=null){
  //if no namespace list supplied get it from fedora_repository module's varaiables
  if ($pid_namespaces==null) {
    $pid_namespaces = array();
    foreach (explode(' ', trim(variable_get('fedora_pids_allowed', 'default: demo: changeme: Islandora: ilives: ')))as $namespace) {
      $pid_namespaces[$namespace] = $namespace;
    }
  }
  
  $collections = array();
  foreach($existing_collections as $collection => $value){
    foreach($pid_namespaces as $key => $namespace){
      if(strpos($collection,$namespace)===0){
        $collections[$collection]=$value;
      }
    }
  }
  return $collections;
}


 /**
 *This function executes a query on Fedora's resource index
 * @param string $itql_query
 *   A query to use for searching the index
 * @return array $list_of_objects
 *   a nice array of objects
 */
function get_related_objects($itql_query) {
  
  module_load_include('inc', 'fedora_repository', 'CollectionClass');
  
  $collection_class = new CollectionClass();
  $query_results = $collection_class->getRelatedItems(NULL, $itql_query);
  $list_of_objects = itql_to_array($query_results);
  return $list_of_objects;
}

/**
 *This function turns an itql result into a usefull array
 * @author
 *   Paul Pound
 * @author
 *   William Panting
 * @param string $query_results
 *   The ugly string version
 * @return array $list_of_objects
 *   The well formed array version
 */
function itql_to_array($query_results) {
  try {
    $xml = new SimpleXMLElement($query_results);
  } catch (Exception $e) {
    drupal_set_message(t('Error getting list of collection objects !e', array('!e' => $e->getMessage())), 'error');
    return;
  }
  $list_of_objects = array();
  foreach ($xml->results->result as $result) {
    $a = $result->object->attributes();
    $temp = $a['uri'];
    $object = substr($temp, strrpos($temp, '/') + 1);
    $title = $result->title;
    $list_of_objects[$object] = (string) $title; //.' '.$object;
  }
  return $list_of_objects;
}

/**
 * This function gets all the members of a collection through the relationship of 'isMemberOf' 
 * and 'isMemberOfCollection' the two relationships need to be checked because there is no
 * Fedora enforced standard.
 * @author
 *   William Panting
 * @param array $collection_id
 *   The collection to get the members of.
 * @return array $member_list_full
 *   The array containing all the pids of members of the collection.
 */
function get_all_members_of_collection($collection_id) {
  
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection'); 
  
  $member_list_full=array();
  
  $member_list1=get_related_items_as_array($collection_id, 'isMemberOf', 10000 , 0, FALSE);
  $member_list2=get_related_items_as_array($collection_id, 'isMemberOfCollection', 10000 , 0, FALSE);
  
  $member_list_full=array_merge($member_list1, $member_list2);
  
  return $member_list_full;
}


/*
* Operations that affect a Fedora repository at a collection level.
*/

module_load_include('inc', 'fedora_repository', 'CollectionClass');
module_load_include('inc', 'fedora_repository', 'api/fedora_item');
module_load_include('inc', 'fedora_repository', 'api/fedora_utils');
module_load_include('module', 'fedora_repository');

/**
* Exports a fedora collection object and all of its children in a format
* that will let you import them into another repository.
* @param <type> $format
*/
function export_collection($collection_pid, $relationship = 'isMemberOfCollection', $format = 'info:fedora/fedora-system:FOXML-1.1' ) {
  $collection_item = new Fedora_Item($collection_pid);
$foxml = $collection_item->export_as_foxml();

$file_dir = file_directory_path();

// Create a temporary directory to contain the exported FOXML files.
$container = tempnam($file_dir, 'export_');
file_delete($container);
print $container;
if (mkdir($container) && mkdir($container . '/'. $collection_pid)) {
$foxml_dir = $container . '/'. $collection_pid;
$file = fopen($foxml_dir . '/'. $collection_pid . '.xml', 'w');
fwrite($file, $foxml);
    fclose($file);

$member_pids = get_related_items_as_array($collection_pid, $relationship);
foreach ($member_pids as $member) {
$file = fopen($foxml_dir . '/'. $member . '.xml', 'w');
$item = new Fedora_Item($member);
      $item_foxml = $item->export_as_foxml();
fwrite($file, $item_foxml);
fclose($file);
}
if (system("cd $container;zip -r $collection_pid.zip $collection_pid/* >/dev/NULL") == 0) {
header("Content-type: application/zip");
header('Content-Disposition: attachment; filename="' . $collection_pid . '.zip'. '"');
$fh = fopen($container . '/'. $collection_pid . '.zip', 'r');
$the_data = fread($fh, filesize($container . '/'. $collection_pid . '.zip'));
      fclose($fh);
echo $the_data;
}
if (file_exists($container . '/'. $collection_pid)) {
      system("rm -rf $container"); // I'm sorry.
}
}
else {
drupal_set_message("Error creating temp directory for batch export.", 'error');
    return FALSE;
}
return TRUE;
}

/**

@TODO:
This is from islandora 6.x core, it needs to be rewritten to use this api
There is also a problem in using DC fields in querries this needs to be refactored to use things
will not return multiple tupples when the person using the api expects one, or we can just remove all
duplicates before returning the array of results in get_array...


* Returns an array of pids that match the query contained in teh collection
 * object's QUERY datastream or in the suppled $query parameter.
* @param <type> $collection_pid
* @param <type> $query
* @param <type> $query_format R

function get_related_items_as_xml($collection_pid, $relationship = array('isMemberOfCollection'), $limit = 10000, $offset = 0, $active_objects_only = TRUE, $cmodel = NULL, $orderby = '$title') {
module_load_include('inc', 'fedora_repository', 'ObjectHelper');
$collection_item = new Fedora_Item($collection_pid);

global $user;
if (!fedora_repository_access(OBJECTHELPER :: $OBJECT_HELPER_VIEW_FEDORA, $pid, $user)) {
drupal_set_message(t("You do not have access to Fedora objects within the attempted namespace or access to Fedora denied."), 'error');
return array();
}

$query_string = 'select $object $title $content from <#ri>
                             where ($object <dc:title> $title
and $object <fedora-model:hasModel> $content
                             and (';

  if (is_array($relationship)) {
    foreach ($relationship as $rel) {
      $query_string .= '$object <fedora-rels-ext:'. $rel . '> <info:fedora/'. $collection_pid . '>';
      if (next($relationship)) {
$query_string .= ' OR ';
}
}
}
elseif (is_string($relationship)) {
$query_string .= '$object <fedora-rels-ext:'. $relationship . '> <info:fedora/'. $collection_pid . '>';
}
else {
return '';
}

  $query_string .= ') ';
  $query_string .=  $active_objects_only ? 'and $object <fedora-model:state> <info:fedora/fedora-system:def/model#Active>' : '';
  
  if ($cmodel) {
  $query_string .= ' and $content <mulgara:is> <info:fedora/' . $cmodel . '>';
}

  $query_string .= ')
                    minus $content <mulgara:is> <info:fedora/fedora-system:FedoraObject-3.0>
  order by '.$orderby;

  $query_string = htmlentities(urlencode($query_string));
  

  $content = '';
  $url = variable_get('fedora_repository_url', 'http://localhost:8080/fedora/risearch');
  $url .= "?type=tuples&flush=TRUE&format=Sparql&limit=$limit&offset=$offset&lang=itql&stream=on&query=". $query_string;
  $content .= do_curl($url);

  return $content;
}

  function get_related_items_as_array($collection_pid, $relationship = 'isMemberOfCollection', $limit = 10000, $offset = 0, $active_objects_only = TRUE, $cmodel = NULL, $orderby = '$title') {
  $content = get_related_items_as_xml($collection_pid, $relationship, $limit, $offset, $active_objects_only, $cmodel, $orderby);
  if (empty($content)) {
  return array();
  }

  $content = new SimpleXMLElement($content);

  $resultsarray = array();
  foreach ($content->results->result as $result) {
  $resultsarray[] = substr($result->object->attributes()->uri, 12); // Remove 'info:fedora/'.
  }
  return $resultsarray;
  }*/

/**
 * 
 */

  /**
    * Performs the given ITQL query.
    * Might be duplicating code from the Fedora API (I seem to recall something
    *   but with a weird name).
    * 
    * FIXME: Could probably made more fail-safe (avoiding passing directly from the curl call to loadXML, for example.)
    *
     * @author
     *   Adam
    * @param String $query
    * @param Integer $limit
    * @param Integer $offset
    * @return DOMDocument 
    */
function performItqlQuery($query, $limit = -1, $offset = 0) {
       $queryUrl = variable_get('fedora_repository_url', 'http://localhost:8080/fedora/risearch');
       $queryUrl .= "?type=tuples&flush=TRUE&format=Sparql" . (($limit > 0)?("&limit=$limit"):("")) . "&offset=$offset&lang=itql&stream=on&query=" . urlencode($query);
       $doc = DOMDocument::loadXML(do_curl($queryUrl));
       return ((!$doc)?(new DOMDocument()):($doc));
   }
   
   
/**
 * This function will get the collection that the indicated object is a member of
 * @param string $object_id
 *   The id of the object to get the parent of
 * @return mixed $parent
 *   The id of the collection object that contains the $object_id object or FALSE if no parent found
 * @todo:
 *   should return all parents... make it return an array
 *   put in the object class
 */
function get_object_parent($object_id) {
  //init
  $parent_relationship='isMemberOf';
  $parent_relationship_namespace='info:fedora/fedora-system:def/relations-external#';
  module_load_include('raw.inc', 'islandora_fedora_api'); //for getting an object
  $apim_object= new FedoraAPIM();
  $relationships_parser = new DOMDocument();
  $parent=FALSE;
  
  //get relation ship data
  try {
    $relationships=$apim_object->getRelationships($object_id, $parent_relationship_namespace . $parent_relationship);
    //grab realtionship
    $relationships_parser->loadXML($relationships->data);
    $relationship_elements=$relationships_parser->getElementsByTagNameNS($parent_relationship_namespace, $parent_relationship);
    $relationship=$relationship_elements->item(0);
  }
  catch (FedoraAPIRestException $e) {
    return FALSE;
  }
  
  //handle second collecion memberships string if the first wasn't found
  if (empty($relationship)) {
    $parent_relationship='isMemberOfCollection';
    try {
      $relationships=$apim_object->getRelationships($object_id, $parent_relationship_namespace . $parent_relationship);
      //grab realtionship 
      $relationships_parser->loadXML($relationships->data);
      $relationship_elements=$relationships_parser->getElementsByTagNameNS($parent_relationship_namespace, $parent_relationship);
      $relationship=$relationship_elements->item(0);
    }
    catch (FedoraAPIRestException $e) {
      return FALSE;
    }
  }
  
  //handle relationship data
  if (!empty($relationship)) {
    $parent=$relationship->getAttributeNS('http://www.w3.org/1999/02/22-rdf-syntax-ns#', 'resource');
    //cut out 'info:fedora/'
    if (substr($parent, 0, 12)=='info:fedora/') {
      $parent=substr($parent, 12, strlen($parent));
    }
  }
  return $parent;
}