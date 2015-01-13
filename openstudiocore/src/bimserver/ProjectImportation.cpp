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

	ProjectImportation::ProjectImportation(QWidget *parent, bimserver::BIMserverConnection *bc) : 
		QDialog(parent) {
		QGridLayout *mainLayout = new QGridLayout;
		m_introLabel = new QLabel("Please select a project to import: ", this);
		m_listWidget = new QListWidget(this);
		m_okButton = new QPushButton("OK",this);
		m_cancelButton = new QPushButton("Cancel",this);
		m_projectID = -1;
		bimserver::BIMserverConnection *m_bimserverConnector = bc;

		connect(m_okButton, SIGNAL(clicked()),this, SLOT(okButton_clicked()));
		connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

		mainLayout->addWidget(m_introLabel,0,0,1,2);
		mainLayout->addWidget(m_listWidget,1,0,1,2);    	
		mainLayout->addWidget(m_okButton,2,0,1,1);
		mainLayout->addWidget(m_cancelButton,2,1,1,1);
		setLayout(mainLayout);
		setWindowTitle("Import Project");
		
		QString username("admin@bimserver.org");
		QString password("admin");

		connect(this, &ProjectImportation::startConnection, m_bimserverConnector, &BIMserverConnection::login);
		connect(m_bimserverConnector, &BIMserverConnection::listAllProjects, this, &ProjectImportation::processProjectList);
		connect(this, &ProjectImportation::projectIDSelected, m_bimserverConnector, &BIMserverConnection::download);
		
		emit startConnection(username, password);
	}

	ProjectImportation::~ProjectImportation()
	{
	}

	void ProjectImportation::processProjectList(QStringList pList)
	{
		foreach(QString itm, pList)
		{
			m_listWidget->addItem(itm);
		}
	}

	void ProjectImportation::okButton_clicked()
	{
		// (bug fix) if nothing is selected. 

		m_projectID = m_listWidget->currentItem()->text().section(":",0,0);
		
		QMessageBox::information(this, "Project Selected", m_projectID.append(" selected, please wait for it to be imported."));
		
		emit projectIDSelected(m_projectID); 
	}

} // bimserver
} // openstudio
