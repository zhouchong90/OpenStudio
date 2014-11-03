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

#include "ReverseTranslator.hpp"

#include "../model/Model.hpp"
#include "../model/ModelObject.hpp"
#include "../model/ModelObject_Impl.hpp"
#include "../model/Facility.hpp"
#include "../model/Facility_Impl.hpp"
#include "../model/Building.hpp"
#include "../model/Building_Impl.hpp"
#include "../model/BuildingStory.hpp"
#include "../model/BuildingStory_Impl.hpp"
#include "../model/ThermalZone.hpp"
#include "../model/ThermalZone_Impl.hpp"
#include "../model/Space.hpp"
#include "../model/Space_Impl.hpp"
#include "../model/Surface.hpp"
#include "../model/Surface_Impl.hpp"
#include "../model/SubSurface.hpp"
#include "../model/SubSurface_Impl.hpp"
#include "../model/ShadingSurface.hpp"
#include "../model/ShadingSurface_Impl.hpp"
#include "../model/ShadingSurfaceGroup.hpp"
#include "../model/ShadingSurfaceGroup_Impl.hpp"

#include "../utilities/core/Assert.hpp"
#include "../utilities/units/UnitFactory.hpp"
#include "../utilities/units/QuantityConverter.hpp"
#include "../utilities/plot/ProgressBar.hpp"

#include <QThread>
#include <iostream>
#include <QFile>
#include <QTextStream>

namespace openstudio {
namespace bimserver {

  ReverseTranslator::ReverseTranslator()
  {
    m_logSink.setLogLevel(Warn);
    m_logSink.setChannelRegex(boost::regex("openstudio\\.bimserver\\.ReverseTranslator"));
    m_logSink.setThreadId(QThread::currentThread());
  }

  ReverseTranslator::~ReverseTranslator()
  {
  }

  boost::optional<openstudio::model::Model> ReverseTranslator::loadModel(const openstudio::path& path, ProgressBar* progressBar)
  {
    m_progressBar = progressBar;

    m_logSink.setThreadId(QThread::currentThread());

    m_logSink.resetStringStream();

    boost::optional<openstudio::model::Model> result;

    // Do work

    return result;
  }


  std::vector<LogMessage> ReverseTranslator::warnings() const
  {
    std::vector<LogMessage> result;

    for (LogMessage logMessage : m_logSink.logMessages()){
      if (logMessage.logLevel() == Warn){
        result.push_back(logMessage);
      }
    }

    return result;
  }

  std::vector<LogMessage> ReverseTranslator::errors() const
  {
    std::vector<LogMessage> result;

    for (LogMessage logMessage : m_logSink.logMessages()){
      if (logMessage.logLevel() > Warn){
        result.push_back(logMessage);
      }
    }

    return result;
  }

  BIMserverConnection::BIMserverConnection(QObject * parent, const char * bimserverUrl) :
    QObject(parent),
    m_networkManager(new QNetworkAccessManager) {
    m_bimserverURL = QString(bimserverUrl);
  }

  BIMserverConnection::~BIMserverConnection() {
    delete m_networkManager;
  }

  void BIMserverConnection::loadModel() {
    //std::cout << "Hello World";
    QString username("admin@bimserver.org");
    QString password("admin");
    loginRequest(username, password);
  }

  void BIMserverConnection::loginRequest(QString username, QString password) const {
    //construct login json
    QJsonObject parameters;
    parameters["username"] = QJsonValue(username);
    parameters["password"] = QJsonValue(password);
    QJsonObject request;
    request["interface"] = QJsonValue("Bimsie1AuthInterface");
    request["method"] = QJsonValue("login");
    request["parameters"] = parameters;
    QJsonObject loginRequest;
    loginRequest["token"] = "1";
    loginRequest["request"] = request;

    QJsonDocument doc;
    doc.setObject(loginRequest);

    QByteArray loginJson = doc.toJson();

    //setup network connection
    QNetworkRequest qNetworkRequest(m_bimserverURL);
    qNetworkRequest.setRawHeader("Content-Type", "application/json");

    // disconnect all signals from m_networkManager to this
    disconnect(m_networkManager, nullptr, this, nullptr);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &BIMserverConnection::processLoginRequest);
    m_networkManager->post(qNetworkRequest, loginJson);
  }

  void BIMserverConnection::processLoginRequest(QNetworkReply *rep) {
    //extract token from login Request
    QByteArray responseArray = rep->readAll();

    QJsonDocument responseDoc = QJsonDocument::fromJson(responseArray);
    QJsonObject loginResponse = responseDoc.object();
    QJsonObject response = loginResponse["response"].toObject();
    m_token = response["result"].toString();

    //TODO Chong add find serializer and find project method

    //setup download IFC json //TODO Chong setup parameters
    QJsonObject parameters;
    parameters["roid"] = QJsonValue(65539);
    parameters["serializerOid"] = QJsonValue(1703974);
    parameters["showOwn"] = QJsonValue(false);
    parameters["sync"] = QJsonValue(false);
    QJsonObject request;
    request["interface"] = QJsonValue("Bimsie1ServiceInterface");
    request["method"] = QJsonValue("download");
    request["parameters"] = parameters;
    QJsonObject downloadRequest;
    downloadRequest["token"] = QJsonValue(m_token);
    downloadRequest["request"] = request;

    QJsonDocument doc;
    doc.setObject(downloadRequest);

    QByteArray downloadJson = doc.toJson();

    //setup network connection
    QNetworkRequest qNetworkRequest(m_bimserverURL);
    qNetworkRequest.setRawHeader("Content-Type", "application/json");

    // disconnect all signals from m_networkManager to this
    disconnect(m_networkManager, nullptr, this, nullptr);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &BIMserverConnection::processDownloadRequest);
    m_networkManager->post(qNetworkRequest, downloadJson);
  }

  void BIMserverConnection::processDownloadRequest(QNetworkReply *rep) {
    //extract token from login Request
    QByteArray responseArray = rep->readAll();

    QJsonDocument responseDoc = QJsonDocument::fromJson(responseArray);
    QJsonObject downloadResponse = responseDoc.object();
    QJsonObject response = downloadResponse["response"].toObject();
    int actionId = response["result"].toInt();

    //setup get download data IFC json //TODO Chong setup parameters
    QJsonObject parameters;
    parameters["actionId"] = QJsonValue(actionId);
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

    //read JsonObject from QString
    QJsonDocument OSMJsonDoc = QJsonDocument::fromJson(OSMFile.toUtf8());
    QJsonObject osmJsonObj = OSMJsonDoc.object();

    //Build OSM model
    openstudio::model::Model model;

    //read and create the spaces
    QJsonArray osmJsonSpaces = osmJsonObj["Spaces"].toArray();  

    foreach (const QJsonValue & value, osmJsonSpaces) {
      QJsonObject osmJsonSpace = value.toObject();
      //process space
      extractSpace(osmJsonSpace, model);
    }

    //read and create the surfaces
    QJsonArray osmJsonSurfaces = osmJsonObj["Surfaces"].toArray();

    foreach(const QJsonValue & value, osmJsonSurfaces) {
      QJsonObject osmJsonSurface = value.toObject();
      //process surfaces

      extractSurface(osmJsonSurface, model);
    }

    //read and create the subsurfaces
    QJsonArray osmJsonSubSurfaces = osmJsonObj["SubSurfaces"].toArray();

    foreach(const QJsonValue & value, osmJsonSubSurfaces) {
      QJsonObject osmJsonSubSurface = value.toObject();
      //process subSurfaces

      extractSubSurface(osmJsonSubSurfaces, model);
    }

  }

  std::string BIMserverConnection::getName(QJsonValue value) {
    return value.toString().toStdString();
  }

  void BIMserverConnection::extractSpace(const QJsonObject &osmJsonSpace, openstudio::model::Model & model) {
    openstudio::model::Space space(model);
    std::string spaceName = osmJsonSpace["Name"].toString().toStdString();
    space.setName(spaceName);

    //set building story
    //set thermal zone
  }

  void BIMserverConnection::extractSurface(const QJsonObject &osmJsonSurface, openstudio::model::Model &model) {

    boost::optional<std::vector<openstudio::Point3d>> vertices = extractPoints(osmJsonSurface);

    openstudio::model::Surface surface(vertices, model);
    //set name
    std::string surfaceName = getName(osmJsonSurface["Name"]);
    surface.setName(surfaceName);

    //set type
    std::string surfaceType = getName(osmJsonSurface["SurfaceType"]);
    surface.setSurfaceType(surfaceType);

    //set parent space
    std::string parentSpaceName = getName(osmJsonSurface["SpaceName"]);
    openstudio::model::Space parentSpace = model.getModelObjectByName(parentSpaceName);
    surface.setSpace(parentSpace);

    //set outsideBoundaryCondition
    std::string outsideBoundaryCondition = getName(osmJsonSurface["OutsideBoundaryCondition"]);
    surface.setOutsideBoundaryCondition(outsideBoundaryCondition);

    //TODO set outsideBoundaryConditionObject
    //TODO add linking attributes

    //set sunExposure
    std::string sunExposed = getName(osmJsonSurface["SunExposure"]);
    surface.setSunExposure(sunExposed);

    //set windExposure
    std::string windExposed = getName(osmJsonSurface["WindExposure"]);
    surface.setWindExposure(windExposed);
  }

  void BIMserverConnection::extractSubSurface(const QJsonObject &osmJsonSubSurface, openstudio::model::Model & model) {
    return boost::optional<std::vector<openstudio::Point3d>> vertices = extractPoints(osmJsonSubSurface);

    openstudio::model::SubSurface subsurface(vertices, model);

    //set name
    std::string subsurfaceName = getName(osmJsonSubSurface["Name"]);
    subsurface.setName(surfaceName);

    //set type
    std::string subsurfaceType = getName(osmJsonSubSurface["SubSurfaceType"]);
    subsurface.setSubSurfaceType(subsurfaceType);

    //set parent surface

    std::string parentSurfaceName = getName(osmJsonSubSurface["SurfaceName"]);
    openstudio::model::Surface parentSurface = model.getModelObjectByName(parentSurfaceName);
    subsurface.setSurface(parentSurface);

    //TODO set outsideboundary condition object;
  }

  boost::optional<std::vector<openstudio::Point3d>> BIMserverConnection::extractPoints(const QJsonObject &osmJsonElement) {
    std::vector<openstudio::Point3d> vertices;
    
    QJsonArray osmJsonVertices = osmJsonElement["Vertices"].toArray();
    foreach(const QJsonValue & point, osmJsonVertices) {
      QJsonObject osmJsonPoint = point.toObject();
      double x = osmJsonPoint["X"].toDouble();
      double y = osmJsonPoint["Y"].toDouble();
      double z = osmJsonPoint["Z"].toDouble();
      vertices.push_back(openstudio::Point3d(x, y, z));
    }

    return vertices;
  }

} // bimserver
} // openstudio
