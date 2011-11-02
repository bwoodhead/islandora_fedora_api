<?php

/**
 * @file
 * Property base class 
 */

/**
 * Properties base class
 * 
 * PropertyObject is a workaround to allow you to create real properties in php. 
 * PHP properties act like attributes by allowing you to set the variable instead
 * of calling a function.  
 * 
 * From details go to: http://www.php.net/manual/en/language.oop5.properties.php#98267
 */
abstract class PropertyObject {

  private $_construct = FALSE;
  protected $_populated = TRUE;

  /**
   * ?
   */
  protected function _refresh() {
    
  }

  /**
   * Magic get function that catches a function call
   * @param type $name The method name ( ie: $obj->a then a is the name )
   * @return type 
   */
  public function __get($name) {
    $var_name = "_$name";
    if ((!property_exists($this, $var_name) || $this->$var_name == NULL) && !$this->_populated) {
      $this->_refresh();
    }
    if (method_exists($this, ($method = 'get_' . $name))) {
      return $this->$method();
    }
    else {
      return;
    }
  }

  /**
   * Isset
   * @param type $name
   * @return type 
   */
  public function __isset($name) {
    if (method_exists($this, ($method = 'isset_' . $name))) {
      return $this->$method();
    }
    else {
      return;
    }
  }

  /**
   * Magic set function that will catch assignments and call a function using the assignment
   * @param type $name The name of the variable to assign a value to
   * @param type $value The value
   */
  public function __set($name, $value) {
    if (method_exists($this, ($method = 'set_' . $name))) {
      $this->$method($value);
    }
    else {
      $this->$name = $value;
    }
  }

  /**
   * Unset 
   * @param type $name 
   */
  public function __unset($name) {
    if (method_exists($this, ($method = 'unset_' . $name))) {
      $this->$method();
    }
  }

}
