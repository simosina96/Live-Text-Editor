//
// Created by Eugenio Marinelli on 2019-08-21.
//

#include "MainWindow.h"

MainWindow::MainWindow(QString siteId)
        : editor(new Editor(siteId,this)),
          login(new Login(this)),
          client(new Client(this)),
          crdt(new CRDT(siteId)),
          controller(new Controller(this->crdt,this->editor,this->client)),
          connection(new Connection(this))
{
    this->login->setClient(this->client);
    connect(this->connection, SIGNAL(connectToAddress(QString)),this, SLOT(connectClient(QString)));

    //this->client->connectTo("127.0.0.1");
   // this->client->requestForFile("CiaoTy!");
}

void MainWindow::show() {
    this->connection->show();
}

void MainWindow::showEditor(){
    this->login->close();
    this->editor->show();
}

void MainWindow::connectClient(QString address) {
    this->client->connectTo(address);
    this->connection->close();
    this->login->show();
}