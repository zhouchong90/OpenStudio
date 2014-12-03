#include "ProjectImportation.hpp"
#include "BIMserverConnection.hpp"
#include "ReverseTranslator.hpp"

#include <QString>
#include <QGridLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QListView>

namespace openstudio {
namespace bimserver {

	ProjectImportation::ProjectImportation(QWidget *parent) : 
		QDialog(parent) {
		QGridLayout *mainLayout = new QGridLayout;
    introLabel = new QLabel("Please select a project to import: ", this);
   	listWidget = new QListWidget(this);
   	okButton = new QPushButton("OK",this);
   	cancelButton = new QPushButton("Cancel",this);
   	projectID = -1;

   	connect(okButton, SIGNAL(clicked()),this, SLOT(okButton_clicked()));
   	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

   	mainLayout->addWidget(introLabel,0,0,1,2);
   	mainLayout->addWidget(listWidget,1,0,1,2);    	
   	mainLayout->addWidget(okButton,2,0,1,1);
    mainLayout->addWidget(cancelButton,2,1,1,1);
    setLayout(mainLayout);
    setWindowTitle("Import Project");
		
		BIMserverConnection *m_bimserverConnector = new BIMserverConnection(nullptr,"127.0.0.1:8082");
		QString username("admin@bimserver.org");
		QString password("admin");
		connect(m_bimserverConnector, &BIMserverConnection::listAllProjects, this, &ProjectImportation::processProjectList);
		m_bimserverConnector->login(username,password);
	}

	ProjectImportation::~ProjectImportation()
	{
	}

	void ProjectImportation::processProjectList(QStringList pList)
	{
    	foreach(QString itm, pList)
    	{
    	    listWidget->addItem(itm);
    	}
	}

	void ProjectImportation::processOSMString(QString osmString)
	{
    ReverseTranslator reverseTranslator;
		boost::optional<openstudio::model::Model> model = reverseTranslator.loadModel(osmString::toStdString());
		translatorErrors = trans.errors();
    translatorWarnings = trans.warnings();

    if( model ) {
    	bool wasQuitOnLastWindowClosed = this->quitOnLastWindowClosed();
      this->setQuitOnLastWindowClosed(false);

      if( m_osDocument )
      {
        if( !closeDocument() ) { 
          this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);
          return;
        }
        processEvents();
      }

      m_osDocument = std::shared_ptr<OSDocument>( new OSDocument(componentLibrary(), 
                                                                   hvacComponentLibrary(), 
                                                                   resourcesPath(), 
                                                                   *model) );
      m_osDocument->markAsModified();
      connect(m_osDocument.get(), &OSDocument::closeClicked, this, &OpenStudioApp::onCloseClicked);
      connect(m_osDocument.get(), &OSDocument::exitClicked, this, &OpenStudioApp::quit);
      connect(m_osDocument.get(), &OSDocument::importClicked, this, &OpenStudioApp::importIdf);
      connect(m_osDocument.get(), &OSDocument::importgbXMLClicked, this, &OpenStudioApp::importgbXML);
      connect(m_osDocument.get(), &OSDocument::importSDDClicked, this, &OpenStudioApp::importSDD);
      connect(m_osDocument.get(), &OSDocument::loadFileClicked, this, &OpenStudioApp::open);
      connect(m_osDocument.get(), &OSDocument::osmDropped, this, &OpenStudioApp::openFromDrag);
      connect(m_osDocument.get(), &OSDocument::loadLibraryClicked, this, &OpenStudioApp::loadLibrary);
      connect(m_osDocument.get(), &OSDocument::newClicked, this, &OpenStudioApp::newModel);
      connect(m_osDocument.get(), &OSDocument::helpClicked, this, &OpenStudioApp::showHelp);
      connect(m_osDocument.get(), &OSDocument::aboutClicked, this, &OpenStudioApp::showAbout);
      m_startupView->hide();

      bool errorsOrWarnings = false;

      QString log;

      for( const auto & error : translatorErrors )
      {
        errorsOrWarnings = true;

        log.append(QString::fromStdString(error.logMessage()));
        log.append("\n");
        log.append("\n");
      }

      for( const auto & warning : translatorWarnings )
      {
        errorsOrWarnings = true;

        log.append(QString::fromStdString(warning.logMessage()));
        log.append("\n");
        log.append("\n");
      }

      if (errorsOrWarnings){
        QMessageBox messageBox; // (parent); ETH: ... but is hidden, so don't actually use
        messageBox.setText("Errors or warnings occurred on " + fileExtension + " import.");
        messageBox.setDetailedText(log);
        messageBox.exec();
      }

      this->setQuitOnLastWindowClosed(wasQuitOnLastWindowClosed);

    }else{

      QMessageBox messageBox; // (parent); ETH: ... but is hidden, so don't actually use
      messageBox.setText("Could not import SDD file.");
      messageBox.setDetailedText(QString("Could not import " + fileExtension + " file at ") + fileName);
      messageBox.exec();
    }
	}

	void ProjectImportation::okButton_clicked()
	{
    	projectID = std::stoi(listWidget->currentItem()->text().section(":",0,0));
    	if(projectID > 0) {
    		connect(m_bimserverConnector,&BIMserverConnection::osmStringRetrieved, this, &ProjectImportation::processOSMString);
    		m_bimserverConnector->download(projectID);
    	} else {
    		OS_ASSERT(false);
    	}	
	}

} // bimserver
} // openstudio
