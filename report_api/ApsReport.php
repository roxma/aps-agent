<?php

class ApsReportData{

    private $beginTime;

    private $sys1;
    private $mod1;
    private $sys2;
    private $mod2;
    private $ip;
    private $port;
    private $code;
    private $result;

    // 如果 ApsReport::init 初始化设置了默认 sys1 和 mod1，就可以不设置这两个参数。
    function setSys1($val)   { $this->sys1 = $val; return $this; }
    function setMod1($val)   { $this->mod1 = $val; return $this; }

    function setSys2($val)   { $this->sys2 = $val; return $this; }
    function setMod2($val)   { $this->mod2 = $val; return $this; }
    function setIp($val)     { $this->ip = $val; return $this; }
    function setPort($val)   { $this->port = $val; return $this; }
    function setCode($val)   { $this->code = $val; return $this; }
    function setResult($val) { $this->result = $val; return $this; }

    function getSys1()      { return $this->sys1; }
    function getMod1()      { return $this->mod1; }
    function getSys2()      { return $this->sys2; }
    function getMod2()      { return $this->mod2; }
    function getIp()        { return $this->ip; }
    function getPort()      { return $this->port; }
    function getCode()      { return $this->code; }
    function getResult()    { return $this->result; }
    function getBeginTime() { return $this->beginTime; }

    function resetBeginTime() { $this->beginTime  = microtime(true); return $this; }

    function __construct() {

        $this->beginTime = microtime(true); 

        $this->sys1   = ApsReport::getDefaultSys1();
        $this->mod1   = ApsReport::getDefaultMod1();
        $this->sys2   = "unknown";
        $this->mod2   = "unknown";
        $this->ip     = "ip";
        $this->port   = 0;
        $this->code   = 0;
        $this->result = null;
    } 


}

// class ApsReportInit()

class ApsReport {

    static private $defaultSys1 = "unknown";
    static private $defaultMod1 = "unknown";

    static function getDefaultSys1() {
        return self::$defaultSys1;
    }

    static function getDefaultMod1() {
        return self::$defaultMod1;
    }

    static function init($defaultSys1, $defaultMod1) {
        self::$defaultSys1 = $defaultSys1;
        self::$defaultMod1 = $defaultMod1;
    }
    

    static function report($d) {

        $sys1   = $d->getSys1();
        $mod1   = $d->getMod1();
        $sys2   = $d->getSys2();
        $mod2   = $d->getMod2();
        $ip     = $d->getIp();
        $port   = $d->getPort();
        $code   = $d->getCode();
        $result = $d->getResult();
        $timeMs = intval((microtime(true)-$d->getBeginTime())*1000000);

        if($result===null) {
            $result = $code==0?1:0;
        }

        static $socket = null;
        if(!$socket) {
            $socket = @socket_create(AF_UNIX, SOCK_DGRAM,getprotobyname('unix'));
            if(!$socket) {
                return false;
            }
        }

        $buf = "{$sys1}\t{$mod1}\t{$sys2}\t{$mod2}\t{$ip}\t{$port}|{$timeMs}\t{$result}\t{$code}";

        $ret = @socket_sendto($socket, $buf, strlen($buf), 0, '/run/aps-agent/APS-AGENT.AF_UNIX.SOCK_DGRAM');
        return $ret;
    }
}


// // 初始化
// ApsReport::init("mysys","mymod");
// 
// 
// // 开始计时
// $rpt  = new ApsReportData();
// $rpt->setSys2("his_sys2")->setMod2("his_mod2");
// 
// 
// // user code
// sleep(1);
// 
// 
// // 上报结果，一般来讲非 0 表示失败，如果非 0 错误码也是成功，请加上 setResult(true)
// $rpt->setSys1("his_sys2")->setMod2("his_mod2")->setIp("127.0.0.1")->setPort(10011)->setCode(0);
// ApsReport::report($rpt);


?>
