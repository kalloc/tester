#!/usr/bin/php
<?
$CheckHTTP=Array();
$CheckHTTP[]=array('check','1blablablaflkdsjfklsdjfl');
$data[]='2blablablaflkdsjfklsdjfl';
$data[]='3blablablaflkdsjfklsdjfl';
$data[]='4blablablaflkdsjfklsdjfl';




class Task {

    private $buf='';
    public function make() {
        $packed='';
	foreach($this->prepare() as $value) {
	    $packed.=pack('i',strlen($value)).$value;
	}
	return pack('i',strlen($packed)).$packed;
    }

}


class Task_HTTP extends Task{

    private $data=array();
    
    public function __construct() {
	$this->clean();
    }

    public function setAuth($login=false,$password=false) {
	if($login) {
	    $this->data['login']=$login;
	    $this->data['password']=$password;
	} else {
	    $this->data['login']=false;
	    $this->data['password']=false;
	}
	return $this;
    }
    public function setCheckWord($word,$condition=true) {
	$this->data['word']=$word;
	$this->data['condition']=$condition;
	return $this;
    }
    public function setUri($uri) {
	$this->data['uri']=$uri;
	return $this;
    }
    public function setMethod($method) {
	$this->data['method']=$method;
	return $this;
    }
    public function setPost() {
	$this->data['method']="POST";
	return $this;
    }

    public function setGet() {
	$this->data['method']="GET";
	return $this;
    }


    public function setHead() {
	$this->data['method']="HEAD";
	return $this;
    }
    public function prepare() {
	return $this->data;
    }
    public function clean() {
	$this->data=array(
		'method'=>'GET',
		'uri'=>'/',
		'word'=>'HTTP/1.',
		'condition'=>true,
		'login'=>false,
		'password'=>false
	);
	return $this;
    }

}

$http = new Task_HTTP();
var_dump($http->clean()->setPost()->make());
var_dump(unPackData($http->make()));
var_dump($http->clean()->setHead()->setCheckWord('найди меня')->setUri('/dfdsfdfdsfds/')->setAuth('login','dfsfdsf')->make());
var_dump(unPackData($http->make()));

function unPackData($packed) {
    $data=array();
    list(,$len)=unpack('i',$packed);
    $packed=substr($packed,4);
    for($i=0;$i<$len;) {
	list(,$blen)=unpack('i',$packed);
	$data[]=substr($packed,4,$blen);
	$packed=substr($packed,4+$blen);
	$i+=$blen+4;
    }
    return $data;
}
