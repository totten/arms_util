<?php

/** 
 * A placeholder which flags an expression as invalid
 */
define(ARMS_UTIL_SELECT_INVALID_EXPR, '!)invalid');

/**
 * An abstract representation of an SQL SELECT query. Note that this class is agnostic to DB API;
 * it can work with any DB API that accepts arbitrary SQL.
 */
class ARMS_Util_Select {
  /** Array of string */
  var $_selects;

  /** String */
  var $_from;

  /** Array of string */
  var $_joins;

  /** Array of string */
  var $_wheres;

  /** Array of string */
  var $_groupBys;

  /** Array of string */
  var $_orderBys;

  /** Int or FALSE */
  var $_limit;

  /** Int or FALSE */
  var $_offset;
  
  function __construct($from) {
    $this->_from = $from;
    $this->_selects = array();
    $this->_joins = array();
    $this->_wheres = array();
    $this->_groupBys = array();
    $this->_orderBys = array();
    $this->_limit = FALSE;
    $this->_offset = FALSE;
  }

  /**
   * Generate a SQL query string for this SELECT query
   *
   * @return string, SQL
   */
  function toSQL() {
    $sql = 'select ' . implode(', ', $this->_selects);
    $sql .= ' from ' . $this->_from;
    $sql .= ' ' . implode(' ', $this->_joins);
    if (!empty($this->_wheres)) {
      $sql .= ' where (';
      $sql .= implode(') and (', $this->_wheres);
      $sql .= ')';
    }
    if (!empty($this->_groupBys)) {
      $sql .= ' group by ';
      $sql .= implode(' ', $this->_groupBys);
    }
    if (!empty($this->_orderBys)) {
      $sql .= ' order by ';
      $sql .= implode(', ', $this->_orderBys);
    }
    if (is_numeric($this->_limit)) {
      $sql .= ' limit ' . $this->_limit;
    }
    if (is_numeric($this->_offset)) {
      $sql .= ' offset ' . $this->_offset;
    }
    return $sql;
  }

  /**
   * Generate a SQL query string that can count the number of records
   * that will be returned by this query.
   *
   * @param $var string optional name of the pseudo-column that will contain the count
   * @return string, SQL
   */
  function toCountSQL($var = 'cnt') {
    $sql = 'select count(*) as ' . $var; 
    $sql .= ' from ' . $this->_from;
    $sql .= ' ' . implode(' ', $this->_joins);
    if (!empty($this->_wheres)) {
      $sql .= ' where (';
      $sql .= implode(') and (', $this->_wheres);
      $sql .= ')';
    }
    if (!empty($this->_groupBys)) {
      $sql .= ' group by ';
      $sql .= implode(' ', $this->_groupBys);
    }
    return $sql;
  }
  
  function addSelect($expr, $skip_invalid = TRUE) {
    if ($expr != ARMS_UTIL_SELECT_INVALID_EXPR || !$skip_invalid) {
      $this->_selects[] = $expr;
    }
    return $this;
  }
  
  function addSelects($exprs, $skip_invalid = TRUE) {
    foreach ($exprs as $expr) {
      $this->addSelect($expr, $skip_invalid);
    }
    return $this;
  }
  
  function addJoin($expr, $skip_invalid = TRUE) {
    if ($expr != ARMS_UTIL_SELECT_INVALID_EXPR || !$skip_invalid) {
      $this->_joins[$expr] = $expr;
    }
    return $this;
  }
  
  function addJoinf($expr, $skip_invalid = TRUE) {
    $args = func_get_args();
    $this->addJoin(call_user_func_array('arms_util_query_sprintf', $args));
    return $this;
  }
  
  /**
   * Select the content of a custom-data field (as well as any joins)
   *
   * @param $entityExpr the join field with the entity's id, e.g. "civicrm_activity.id" or "civicrm_activity.source_contact_id"
   * @param $field array{civicrm_custom_field} as formatted by arms_util_field()
   * @return $this
   */
  function addCustomField($entityExpr, $field, $fieldAlias = FALSE, $tableAlias = FALSE) {
    if ($fieldAlias == FALSE) {
      $fieldAlias = 'custom_' . $field['id'];
    }
    if ($tableAlias  == FALSE) {
      $tableAlias = $field['_table_name'];
    }
    $join = sprintf('LEFT JOIN %s %s ON %s = %s.entity_id', $field['_table_name'], $tableAlias, $entityExpr, $tableAlias);
    $this->addJoin($join);
    $this->addSelect(sprintf('%s.%s AS %s', $tableAlias, $field['column_name'], $fieldAlias));
    return $this;
  }
  
  function addWhere($expr, $skip_invalid = TRUE) {
    if ($expr != ARMS_UTIL_SELECT_INVALID_EXPR || !$skip_invalid) {
      $this->_wheres[] = $expr;
    }
    return $this;
  }

  /**
   * Add a formatted where condition
   *
   * Example:
   * $sql->addWheref('favorite_phrase = "%s"', "All the world's a stage");
   */
  function addWheref() {
    $args = func_get_args();
    $this->addWhere(call_user_func_array('arms_util_query_sprintf', $args));
    return $this;
  }
  
  function addGroupBy($expr, $skip_invalid = TRUE) {
    if ($expr != ARMS_UTIL_SELECT_INVALID_EXPR || !$skip_invalid) {
      $this->_groupBys[] = $expr;
    }
    return $this;
  }
  
  function addOrderBy($expr, $skip_invalid = TRUE) {
    if ($expr != ARMS_UTIL_SELECT_INVALID_EXPR || !$skip_invalid) {
      $this->_orderBys[] = $expr;
    }
    return $this;
  }
  
  function setPage($offset, $limit) {
    $this->_offset = $offset;
    $this->_limit = $limit;
  }
}

/**
 * An abstract representation of an SQL UPDATE query. Note that this class is agnostic to DB API;
 * it can work with any DB API that accepts arbitrary SQL.
 */
class ARMS_Util_Update {
  /** Array of string */
  var $_updates;

  /** String */
  var $_table;

  /** Array of string */
  var $_wheres;

  function __construct($table) {
    $this->_table = $table;
    $this->_updates = array();
    $this->_wheres = array();
  }

  /**
   * Generate a SQL query string for this SELECT query
   *
   * @return string, SQL
   */
  function toSQL() {
    if (empty($this->_updates)) {
      return '';
    }
    $sql = 'update ' . $this->_table . ' set ';
    $sql .= implode(', ', $this->_updates);
    if (!empty($this->_wheres)) {
      $sql .= ' where (';
      $sql .= implode(') and (', $this->_wheres);
      $sql .= ')';
    }
    return $sql;
  }
  
  function addUpdate($expr, $skip_invalid = TRUE) {
    if ($expr != ARMS_UTIL_SELECT_INVALID_EXPR || !$skip_invalid) {
      $this->_updates[] = $expr;
    }
    return $this;
  }
  
  /**
   * Add a formatted SET phrase
   *
   * Example:
   * $sql->addUpdatef('favorite_phrase = "%s"', "All the world's a stage");
   */
  function addUpdatef() {
    $args = func_get_args();
    $this->addUpdate(call_user_func_array('arms_util_query_sprintf', $args));
  }
  
  function addUpdates($exprs, $skip_invalid = TRUE) {
    foreach ($exprs as $expr) {
      $this->addUpdate($expr, $skip_invalid);
    }
    return $this;
  }
  
  /**
   * Set the cell value for a given column to a string or number
   */
  function addValue($column, $value) {
    if ($value === NULL || $value === FALSE) {
      return $this->addUpdate($column . ' = NULL');
    } else {
      return $this->addUpdate($column . ' = '.  arms_util_query_sprintf('"%s"', $value));
    }
  }
  
  /**
   * Set the values for several cells
   *
   * @param $values array(sqlColumn => stringOrNumber)
   */
  function addValues($values) {
    foreach ($values as $column => $value) {
      $this->addValue($column, $value);
    }
    return $this;
  }
  
  function addWhere($expr, $skip_invalid = TRUE) {
    if ($expr != ARMS_UTIL_SELECT_INVALID_EXPR || !$skip_invalid) {
      $this->_wheres[] = $expr;
    }
    return $this;
  }
  
  /**
   * Add a formatted where condition
   *
   * Example:
   * $sql->addWheref('favorite_phrase = "%s"', "All the world's a stage");
   */
  function addWheref() {
    $args = func_get_args();
    return $this->addWhere(call_user_func_array('arms_util_query_sprintf', $args));
  }

}

/**
 * An abstract representation of an SQL INSERT query. Note that this class is agnostic to DB API;
 * it can work with any DB API that accepts INSERT ON DUPLICATE KEY UPDATE statements.
 */
class ARMS_Util_Insert {
  /** Array(columnName => sqlExpr) */
  var $_insertExprs;

  /** Array(columnName => sqlExpr) */
  var $_updateExprs;

  /** String */
  var $_table;

  /**
   * @param $table string, SQL table name
   * @param $dupMode string, one of:
   *   - FALSE: no duplicate checking
   *   - 'update': use "INSERT ... ON DUPLICATE KEY UPDATE" mode
   *   - 'ignore': use "INSERT IGNORE INTO ..."
   */
  function __construct($table, $dupMode = FALSE) {
    $this->_table = $table;
    $this->_insertExprs = array();
    $this->_updateExprs = array();
    $this->_dupMode = $dupMode;
    $this->_keyColumns = array();
  }

  /**
   * Generate a SQL query string for this SELECT query
   *
   * @return string, SQL
   * @deprecated
   */
  function toSQL() {
    if (empty($this->_insertExprs)) {
      return '';
    }
    list ($sql, $args) = $this->toDrupalParts();
    return arms_util_query_sprintf($sql, $args);
  }
  
  /**
   * @return array(0=>SQL, 1=>array)
   */
  function toDrupalParts() {
    if (empty($this->_insertExprs)) {
      return '';
    }
    $args = array();
    $sql = 'INSERT ';
    if ($this->_dupMode == 'ignore') {
      $sql .= ' IGNORE ';
    }
    $sql .= ' INTO ' . $this->_table . ' (';
    $sql .= implode(",\n", array_keys($this->_insertExprs));
    $sql .= ') VALUES (';
    
    $first = TRUE;
    foreach ($this->_insertExprs as $expr) {
      if (! $first) $sql .= ",\n";
      if (is_array($expr) && $expr['escape']) {
        $sql .= '"%s"';
        $args[] = $expr['value'];
      } elseif (is_array($expr) && !$expr['escape']) {
          $sql .= $expr['value'];
      } else {
        $sql .= $expr;
      }
      $first = FALSE;
    }
    $sql .= ') ';
    if ($this->_dupMode == 'update' && !empty($this->_updateExprs)) {
      $sql .= ' ON DUPLICATE KEY UPDATE ';
      $first = TRUE;
      foreach ($this->_updateExprs as $column => $expr) {
        if (!$first) $sql .=  ",\n";
        $sql .= $column . ' = ';
        if (is_array($expr) && $expr['escape']) {
          $sql .= '"%s"';
          $args[] = $expr['value'];
        } elseif (is_array($expr) && !$expr['escape']) {
            $sql .= $expr['value'];
        } else {
          $sql .= $expr;
        }
        $first = FALSE;
      }
    }
    return array($sql, $args);
  }
  
  function toDrupalQuery() {
    list ($sql, $args) = $this->toDrupalParts();
    return db_query($sql, $args);
  }
  
  /**
   * Set the cell value for a given column using an SQL expression
   *
   * @param $mode string, 'insert-only', 'insert-update', 'update-only'; use FALSE for auto
   */
  function addExpr($column, $expr, $mode = FALSE, $skip_invalid = TRUE) {
    if ($mode == FALSE) {
      $mode = 'insert-update';
    }
    if ($expr != ARMS_UTIL_SELECT_INVALID_EXPR || !$skip_invalid) {
      if ($mode == 'insert-only' || $mode == 'insert-update') {
        $this->_insertExprs[$column] = $expr;
      }
      if ($this->_dupMode == 'update' && ($mode == 'update-only' || $mode == 'insert-update')) {
        $this->_updateExprs[$column] = $expr;
      }
    }
    return $this;
  }
  
  /**
   * Set the cell value for a given column to a string or number
   *
   * @param $mode string, 'insert-only', 'insert-update', 'update-only'
   */
  function addValue($column, $value, $mode = FALSE) {
    if ($value === NULL || $value === FALSE) {
      return $this->addExpr($column, 'NULL', $mode);
    } else {
      return $this->addExpr($column, array('escape'=>1,'value'=>$value), $mode);
    }
  }
  
  /**
   * Set the values for several cells
   *
   * @param $values array(sqlColumn => stringOrNumber)
   * @param $mode string, 'insert-only', 'insert-update', 'update-only'
   */
  function addValues($values, $mode = FALSE) {
    foreach ($values as $column => $value) {
      $this->addValue($column, $value, $mode);
    }
    return $this;
  }
}

/**
 * Instantiate a new SELECT query
 */
function arms_util_query($from) {
  return new ARMS_Util_Select($from);
}

/**
 * Instantiate a new UPDATE query
 */
function arms_util_update($from) {
  return new ARMS_Util_Update($from);
}

/**
 * Instantiate a new INSERT query
 *
 * @param $dupMode string, one of:
 *   - FALSE: no duplicate checking
 *   - 'update': use "INSERT ... ON DUPLICATE KEY UPDATE" mode
 *   - 'ignore': use "INSERT IGNORE INTO ..."
 */
function arms_util_insert($from, $mode = FALSE) {
  return new ARMS_Util_Insert($from, $mode);
}

/** 
 * Create a SQL "in" condition
 *
 * @param $values array of scalar
 * @param $escape boolean optional; whether the scalars in $values must be escaped
 */
function arms_util_query_in($column, $values, $escape = TRUE) {
    if (empty($values)) {
      return ARMS_UTIL_SELECT_INVALID_EXPR;
    }
    if ($escape) {
      return $column . ' in ("' . implode('","', array_map('db_escape_string', $values)) . '")';
    } else {
      return $column . ' in (' . implode(',', $values) . ')';
    }
}

/** 
 * Create a SQL "in" condition
 *
 * @param $values array of scalar
 * @param $escape boolean optional; whether the scalars in $values must be escaped
 */
function arms_util_query_not_in($column, $values, $escape = TRUE) {
    if (empty($values)) {
      return ARMS_UTIL_SELECT_INVALID_EXPR;
    }
    if ($escape) {
      return $column . ' not in ("' . implode('","', array_map('db_escape_string', $values)) . '")';
    } else {
      return $column . ' not in (' . implode(',', $values) . ')';
    }
}

/**
 * Create a SQL "or" expression
 *
 * @param $exprs array of string
 * @return ARMS_UTIL_SELECT_INVALID_EXPR or string
 */
function arms_util_query_or($exprs) {
  if (!is_array($exprs) || empty($exprs)) {
    return ARMS_UTIL_SELECT_INVALID_EXPR;
  }
  return '(' . implode(') or (', $exprs) . ')';
}

/**
 * Generate a SQL "between" condition
 *
 * This may not produce a literal 'between'. In fact, if $low or $high is omitted, the expression
 * may be unbounded on one side.
 *
 * @param $column string
 * @param $low scalar optional, minimum acceptable value (inclusive); if omitted, then there is no lower bound
 * @param $high scalar optional, maximum acceptable value (inclusive); if omitted, then there is no upper bound
 * @param $escape boolean optional; whether the scalars in $low or $high must be escaped
 * @return ARMS_UTIL_SELECT_INVALID_EXPR or string
 */
function arms_util_query_between($column, $low, $high, $escape = TRUE) {
  if (_arms_util_query_blank($low) && _arms_util_query_blank($high)) {
    return ARMS_UTIL_SELECT_INVALID_EXPR;
  }
  
  $result = array();
  if (!_arms_util_query_blank($low)) {
    if ($escape) {
      $result[] = $column . ' >= "' . db_escape_string($low) . '"';
    } else {
      $result[] = $column . ' >= ' . $low;
    }
  } 
  if (!_arms_util_query_blank($high)) {
    if ($escape) {
      $result[] = $column . ' <= "' . db_escape_string($high) . '"';
    } else {
      $result[] = $column . ' <= ' . $high;
    }
  }
  return join(' and ', $result);
}

/**
 * Generate a SQL condition which is TRUE if and only if $column falls within one of the $betweens
 *
 * This may not produce a literal 'between'. In fact, if $low or $high is omitted, the expression
 * may be unbounded on one side.
 *
 * @param $column string
 * @param $betweens array, required; each item in the array is an array with a (start_date,end_date) tuple
 * @param $escape boolean optional; whether the scalars in $low or $high must be escaped
 * @return ARMS_UTIL_SELECT_INVALID_EXPR or string
 */
function arms_util_query_betweens($column, $betweens, $escape = TRUE) {
  if (empty($betweens)) {
    return ARMS_UTIL_SELECT_INVALID_EXPR;
  }
  $exprs = array();
  foreach ($betweens as $between) {
    $exprs[] = arms_util_query_between($column, $between['start_date'], $between['end_date'], $escape);
  }
  return arms_util_query_or($exprs);
}

/**
 * Determine whether $value is FALSE, NULL, or an empty string. The number 0 is not blank.
 *
 * @return boolean
 */
function _arms_util_query_blank($value) {
  return $value === FALSE || $value === NULL || $value === '';
}

/**
 * Generate a SQL expression in which %s, %d, etc are substituted with values
 *
 * @see db_query
 */
function arms_util_query_sprintf($query) {
  $args = func_get_args();
  array_shift($args);
  $query = db_prefix_tables($query);
  if (isset($args[0]) and is_array($args[0])) { // 'All arguments in one array' syntax
    $args = $args[0];
  }
  _db_query_callback($args, TRUE);
  $query = preg_replace_callback(DB_QUERY_REGEXP, '_db_query_callback', $query);
  return $query;
}

