<?php
/* $Id: ConnectDB_mysql.inc 7751 2017-04-13 16:34:26Z rchacon $ */
/* Database abstraction for mysql */

define('LIKE', 'LIKE');

global $db;// Make sure it IS global, regardless of our context
if(!isset($mysqlport)) {
	$mysqlport = 3306;
}
$db = mysql_connect($host . ':' . $mysqlport, $DBUser, $DBPassword);

//this statement sets the charset to be used for sending data to and from the db server
//if not set, both mysql server and mysql client/library may assume otherwise
mysql_set_charset('utf8', $db);

//$varabc = mysql_client_encoding($db);
//printf("client encoding is %s\n", $varabc);


if(!$db) {
	echo '<br />' . _('The configuration in the file config.php for the database user name and password do not provide the information required to connect to the database server');
	session_unset();
	session_destroy();
	echo '<p>' . _('Click') . ' ' . '<a href="index.php">' . _('here') . '</a>' . ' ' ._('to try logging in again') . '</p>';

	exit;
}

/* Update to allow RecurringSalesOrdersProcess.php to run via cron */
if(isset($DatabaseName)) {

	if(! mysql_select_db($_SESSION['DatabaseName'],$db)) {
		echo '<br />' . _('The company name entered does not correspond to a database on the database server specified in the config.php configuration file. Try logging in with a different company name');
		echo '<br /><a href="index.php">' . _('Back to login page') . '</A>';
		unset($_SESSION['DatabaseName']);
		exit;
	}
} else {
	if(! mysql_select_db($_SESSION['DatabaseName'],$db)) {
		echo '<br />' . _('The company name entered does not correspond to a database on the database server specified in the config.php configuration file. Try logging in with a different company name');
		echo '<br /><a href="index.php">' . _('Back to login page') . '</A>';
		unset($_SESSION['DatabaseName']);
		exit;
	}
}

require_once($PathPrefix . 'includes/MiscFunctions.php');

//DB wrapper functions to change only once for whole application

function DB_query($SQL, $ErrorMessage='', $DebugMessage= '', $Transaction=false, $TrapErrors=true) {

	global $debug;
	global $PathPrefix;
	global $db;

	$result = mysql_query($SQL, $db);
	$_SESSION['LastInsertId'] = mysql_insert_id($db);

	if($DebugMessage == '') {
		$DebugMessage = _('The SQL that failed was');
	}

	if(DB_error_no() != 0 AND $TrapErrors==true) {
		if($TrapErrors) {
			require_once($PathPrefix . 'includes/header.php');
		}
		prnMsg($ErrorMessage . '<br />' . DB_error_msg(), 'error', _('Database Error'));
		if($debug == 1) {
			prnMsg($DebugMessage . '<br />' . $SQL . '<br />', 'error', _('Database SQL Failure'));
			}
		if($Transaction) {
			$SQL = 'rollback';
			$Result = DB_query($SQL);
			if(DB_error_no() != 0) {
				prnMsg(_('Error Rolling Back Transaction'), '', _('Database Rollback Error') );
			}
		}
		if($TrapErrors) {
			include($PathPrefix . 'includes/footer.php');
			exit;
		}
	} elseif(isset($_SESSION['MonthsAuditTrail']) and (DB_error_no()==0 AND $_SESSION['MonthsAuditTrail']>0)) {

		$SQLArray = explode(' ', $SQL);

		if(($SQLArray[0] == 'INSERT')
			OR ($SQLArray[0] == 'UPDATE')
			OR ($SQLArray[0] == 'DELETE')) {

			if($SQLArray[2] != 'audittrail') { // to ensure the auto delete of audit trail history is not logged
				$AuditSQL = "INSERT INTO audittrail (transactiondate,
									userid,
									querystring)
						VALUES('" . Date('Y-m-d H:i:s') . "',
							'" . trim($_SESSION['UserID']) . "',
							'" . DB_escape_string($SQL) . "')";

				$AuditResult = mysql_query($AuditSQL,$db);
			}
		}
	}

	return $result;

}

function DB_fetch_row($ResultIndex) {
	$RowPointer = mysql_fetch_row($ResultIndex);
	return $RowPointer;
}

function DB_fetch_assoc($ResultIndex) {
	$RowPointer = mysql_fetch_assoc($ResultIndex);
	return $RowPointer;
}

function DB_fetch_array($ResultIndex) {
	$RowPointer = mysql_fetch_array($ResultIndex);
	return $RowPointer;
}

function DB_data_seek(&$ResultIndex,$Record) {
	mysql_data_seek($ResultIndex,$Record);
}

function DB_free_result($ResultIndex) {
	mysql_free_result($ResultIndex);
}

function DB_num_rows($ResultIndex) {
	return mysql_num_rows($ResultIndex);
}

function DB_affected_rows($ResultIndex) {
	return mysql_affected_rows($ResultIndex);
}

function DB_error_no() {
	global $db;
	return mysql_errno($db);
}

function DB_error_msg() {
	global $db;
	return mysql_error($db);
}

function DB_Last_Insert_ID(&$Conn='',$table, $fieldname) {
//	return mysql_insert_id($db);
	if (isset($_SESSION['LastInsertId'])) {
		$Last_Insert_ID = $_SESSION['LastInsertId'];
	} else {
		$Last_Insert_ID = 0;
	}
//	unset($_SESSION['LastInsertId']);
	return $Last_Insert_ID;
}

function DB_escape_string($String) {
	return mysql_real_escape_string($String);
}

function DB_show_tables(&$Conn='') {
	$Result = DB_query('SHOW TABLES');
	return $Result;
}

function DB_show_fields($TableName, &$Conn='') {
	$Result = DB_query("DESCRIBE $TableName");
	return $Result;
}

function interval( $val, $Inter ) {
	global $DBType;
	return "\n".'interval ' . $val . ' '. $Inter."\n";
}

function DB_Maintenance() {
	prnMsg(_('The system has just run the regular database administration and optimisation routine.'),'info');
	$TablesResult = DB_query('SHOW TABLES');
	while ($myrow = DB_fetch_row($TablesResult)) {
		$Result = DB_query('OPTIMIZE TABLE ' . $myrow[0]);
	}

	$Result = DB_query("UPDATE config
				SET confvalue='" . Date('Y-m-d') . "'
				WHERE confname='DB_Maintenance_LastRun'");
}

function DB_Txn_Begin() {
	global $db;
	mysql_query("SET autocommit=0",$db);
	mysql_query("START TRANSACTION",$db);
}

function DB_Txn_Commit() {
	global $db;
	mysql_query("COMMIT",$db);
	mysql_query("SET autocommit=1",$db);
}

function DB_Txn_Rollback() {
	global $db;
  mysql_query("ROLLBACK",$db);
}

function DB_IgnoreForeignKeys() {
	global $db;
	mysql_query("SET FOREIGN_KEY_CHECKS=0",$db);
}

function DB_ReinstateForeignKeys() {
	global $db;
	mysql_query("SET FOREIGN_KEY_CHECKS=1",$db);
}

?>
