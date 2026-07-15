
//发送时我们会将发送的消息放入队列里以保证发送的时序性，每个session都有一个发送队列，
//因为有的时候发送的频率过高会导致队列增大，所以要对队列的大小做限制，
//当队列大于指定数量的长度时，就丢弃要发送的数据包，以保证消息的快速收发

void CSession::Send(char* msg, int max_length) {
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE) {
        cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << endl;
        return;
    }

    _send_que.push(make_shared<MsgNode>(msg, max_length));
    if (send_que_size > 0) {
        return;
    }
    auto& msgnode = _send_que.front();
    boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
        std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
}