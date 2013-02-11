void main_loop(){
  poll_connection_pool();
  associate_endpoints();
  associate_idle_connections();

}
