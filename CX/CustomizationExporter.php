<?php

/**
 * This class manages a batch of export tasks. It processes each in turn
 * and then returns an XML document.
 */
class CX_CustomizationExporter {

  var $queue;
  var $done;
  var $xml;
  
  function CX_CustomizationExporter() {
    $this->queue = array();
    $this->done = array();
  }
  
  function createXml() {
    $this->xml = new SimpleXMLElement('<customizations />');
    while (! empty($this->queue)) {
      $task = array_shift($this->queue);
      if (in_array($task, $this->done)) { continue; }
      $this->done []= $task;
      
      #printf("Calling: [%s]\n", print_r($task, true));
      $task->export($this);
    }

    return $this->xml;
  }
  
  function enqueue($task) {
    $this->queue []= $task;
  }
}

?>