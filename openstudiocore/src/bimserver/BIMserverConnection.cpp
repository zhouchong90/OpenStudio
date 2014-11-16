/**********************************************************************
 *  Copyright (c) 2008-2014, Alliance for Sustainable Energy.
 *  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 **********************************************************************/

#include "BIMserverConnection.hpp"

#include <QString>
#include <QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QByteArray>

#include <iostream>


namespace openstudio {
namespace bimserver {

  BIMserverConnection::BIMserverConnection(QObject * parent, const char * bimserverUrl) :
    QObject(parent),
    m_networkManager(new QNetworkAccessManager) {
    m_bimserverURL = QString(bimserverUrl);
  }

  BIMserverConnection::~BIMserverConnection() {
    delete m_networkManager;
  }

  void BIMserverConnection::login(QString username, QString password) {
    //std::cout << "Hello World";
    m_username = username;
    m_password = password;
    sendLoginRequest();
  }

  void BIMserverConnection::sendLoginRequest() {
    //construct login json
    QJsonObject parameters;
    parameters["username"] = QJsonValue(m_username);
    parameters["password"] = QJsonValue(m_password);
    QJsonObject request;
    request["interface"] = QJsonValue("Bimsie1AuthInterface");
    request["method"] = QJsonValue("login");
    request["parameters"] = parameters;
    QJsonObject jsonRequest;
    jsonRequest["token"] = "1";
    jsonRequest["request"] = request;

    QJsonDocument doc;
    doc.setObject(jsonRequest);

    QByteArray jsonByteArray = doc.toJson();

    //setup network connection
    QNetworkRequest qNetworkRequest(m_bimserverURL);
    qNetworkRequest.setRawHeader("Content-Type", "application/json");

    // disconnect all signals from m_networkManager to this
    disconnect(m_networkManager, nullptr, this, nullptr);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &BIMserverConnection::processLoginRequest);
    m_networkManager->post(qNetworkRequest, jsonByteArray);
  }

  void BIMserverConnection::processLoginRequest(QNetworkReply *rep) {
    //extract token from login Request
    QByteArray responseArray = rep->readAll();

    QJsonDocument responseDoc = QJsonDocument::fromJson(responseArray);
    QJsonObject responseObj = responseDoc.object();
    QJsonObject response = responseObj["response"].toObject();
    m_token = response["result"].toString();

    //TODO Chong add find serializer and find project method
    sendGetAllProjectsRequest();
  }

  void BIMserverConnection::sendGetAllProjectsRequest() {
    QJsonObject parameters;
    parameters["onlyTopLevel"] = QJsonValue(true);
    parameters["onlyActive"] = QJsonValue(false);
    QJsonObject request;
    request["interface"] = QJsonValue("Bimsie1ServiceInterface");
    request["method"] = QJsonValue("getAllProjects");
    request["parameters"] = parameters;
    QJsonObject jsonRequest;
    jsonRequest["token"] = QJsonValue(m_token);
    jsonRequest["request"] = request;

    QJsonDocument doc;
    doc.setObject(jsonRequest);

    QByteArray downloadJson = doc.toJson();

    //setup network connection
    QNetworkRequest qNetworkRequest(m_bimserverURL);
    qNetworkRequest.setRawHeader("Content-Type", "application/json");

    // disconnect all signals from m_networkManager to this
    disconnect(m_networkManager, nullptr, this, nullptr);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &BIMserverConnection::processGetAllProjectsRequest);
    m_networkManager->post(qNetworkRequest, downloadJson);
  }

  void BIMserverConnection::processGetAllProjectsRequest(QNetworkReply *rep) {
    QByteArray responseArray = rep->readAll();

    QJsonDocument responseDoc = QJsonDocument::fromJson(responseArray);
    QJsonObject responseObj = responseDoc.object();

    QJsonObject response = responseObj["response"].toObject();
    QJsonArray result = response["result"].toArray();

    QStringList projectList;

    foreach(const QJsonValue & value, result) {
      QJsonObject project = value.toObject();
      int lastRevisionId = project["lastRevisionId"].toInt();
      QString projectName = project["name"].toString();
      
      QString project = QString::number(lastRevisionId).append(";").append(projectName);
      
      projectList.append(project);
    }

    emit listAllProjects(projectList);
  }

  void BIMserverConnection::download(int projectID) {
    m_roid = QString::number(projectID);
    sendGetSerializerRequest();
  }

  void BIMserverConnection::sendGetSerializerRequest() {
    QJsonObject parameters;
    parameters["serializerName"] = QJsonValue("OSMSerializer");
    QJsonObject request;
    request["interface"] = QJsonValue("Bimsie1ServiceInterface");
    request["method"] = QJsonValue("getSerializerByName");
    request["parameters"] = parameters;
    QJsonObject jsonRequest;
    jsonRequest["token"] = QJsonValue(m_token);
    jsonRequest["request"] = request;

    QJsonDocument doc;
    doc.setObject(jsonRequest);

    QByteArray jsonByteArray = doc.toJson();

    //setup network connection
    QNetworkRequest qNetworkRequest(m_bimserverURL);
    qNetworkRequest.setRawHeader("Content-Type", "application/json");

    // disconnect all signals from m_networkManager to this
    disconnect(m_networkManager, nullptr, this, nullptr);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &BIMserverConnection::processGetSerializerRequest);
    m_networkManager->post(qNetworkRequest, jsonByteArray);
  }

  void BIMserverConnection::processGetSerializerRequest(QNetworkReply *rep) {
    QByteArray responseArray = rep->readAll();

    QJsonDocument responseDoc = QJsonDocument::fromJson(responseArray);
    QJsonObject downloadResponse = responseDoc.object();
    QJsonObject response = downloadResponse["response"].toObject();

    QJsonObject result = response["result"].toObject();
    int serializerID = result["oid"].toInt();
    m_serializerOid = QString::number(serializerID);

    sendDownloadRequest();
  }

  void BIMserverConnection::sendDownloadRequest() {
    QJsonObject parameters;
    parameters["roid"] = QJsonValue(m_roid);
    parameters["serializerOid"] = QJsonValue(m_serializerOid);
    parameters["showOwn"] = QJsonValue(false);
    parameters["sync"] = QJsonValue(false);
    QJsonObject request;
    request["interface"] = QJsonValue("Bimsie1ServiceInterface");
    request["method"] = QJsonValue("download");
    request["parameters"] = parameters;
    QJsonObject jsonRequest;
    jsonRequest["token"] = QJsonValue(m_token);
    jsonRequest["request"] = request;

    QJsonDocument doc;
    doc.setObject(jsonRequest);

    QByteArray jsonByteArray = doc.toJson();

    //setup network connection
    QNetworkRequest qNetworkRequest(m_bimserverURL);
    qNetworkRequest.setRawHeader("Content-Type", "application/json");

    // disconnect all signals from m_networkManager to this
    disconnect(m_networkManager, nullptr, this, nullptr);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &BIMserverConnection::processDownloadRequest);
    m_networkManager->post(qNetworkRequest, jsonByteArray);
  }

  void BIMserverConnection::processDownloadRequest(QNetworkReply *rep) {
    //extract token from login Request
    QByteArray responseArray = rep->readAll();

    QJsonDocument responseDoc = QJsonDocument::fromJson(responseArray);
    QJsonObject downloadResponse = responseDoc.object();
    QJsonObject response = downloadResponse["response"].toObject();
    int actionID = response["result"].toInt();
    m_actionId = QString::number(actionID);

    sendGetDownloadDataRequest();
  }

  void BIMserverConnection::sendGetDownloadDataRequest() {
    QJsonObject parameters;
    parameters["actionId"] = QJsonValue(m_actionId);
    QJsonObject request;
    request["interface"] = QJsonValue("Bimsie1ServiceInterface");
    request["method"] = QJsonValue("getDownloadData");
    request["parameters"] = parameters;
    QJsonObject getDownloadDataRequest;
    getDownloadDataRequest["token"] = QJsonValue(m_token);
    getDownloadDataRequest["request"] = request;

    QJsonDocument doc;
    doc.setObject(getDownloadDataRequest);

    QByteArray getDownloadDataJson = doc.toJson();

    //setup network connection
    QNetworkRequest qNetworkRequest(m_bimserverURL);
    qNetworkRequest.setRawHeader("Content-Type", "application/json");

    // disconnect all signals from m_networkManager to this
    disconnect(m_networkManager, nullptr, this, nullptr);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &BIMserverConnection::processGetDownloadDataRequest);
    m_networkManager->post(qNetworkRequest, getDownloadDataJson);
  }

  void BIMserverConnection::processGetDownloadDataRequest(QNetworkReply *rep) {
    //extract token from login Request
    QByteArray responseArray = rep->readAll();

    QJsonDocument responseDoc = QJsonDocument::fromJson(responseArray);
    QJsonObject downloadResponse = responseDoc.object();
    QJsonObject response = downloadResponse["response"].toObject();
    QJsonObject result = response["result"].toObject();
    QString file = result["file"].toString();

    //decode the response
    QByteArray byteArray;
    byteArray.append(file);
    QString OSMFile = QByteArray::fromBase64(byteArray);

    //TODO connect this signal to the GUI slots to call ReverseTranslator

    emit osmStringRetrieved(OSMFile);

  }

  int main(int argc, char *argv[]) {
    
    std::cout << "Hello World";
    getchar();

    return 0;
  }
  

} // bimserver
} // openstudio
