<?php
 /**
 * This function is used to get a list containing all of the islandora collections in a Fedora repo
 * @return $collection_list
 *   an associated array of collection pids and names
 */
function islandora_workflow_get_all_collections() {
  
  $collection_list=array();
  
  //read in the itql query for getting collections
  $query_file_name=drupal_get_path('module', 'islandora_workflow') . '/collection_query.txt';
  $query_file_handle=fopen($query_file_name, "r");
  $query_string=fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);
  //make query
  $collection_list=islandora_workflow_get_related_objects($query_string);
  //strip out non-applicable collections via namespace
  $collection_list=islandora_workflow_limit_collections_by_namespace($collection_list);
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
function islandora_workflow_limit_collections_by_namespace($existing_collections, $pid_namespaces=null){
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
function islandora_workflow_get_related_objects($itql_query) {
  module_load_include('inc', 'fedora_repository', 'CollectionClass');
  $collection_class = new CollectionClass();
  $query_results = $collection_class->getRelatedItems(NULL, $itql_query);
  $list_of_objects = islandora_workflow_itql_to_array($query_results);
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
function islandora_workflow_itql_to_array($query_results) {
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