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
		introLabel = new QLabel("Please select a project to import: ", this);
		listWidget = new QListWidget(this);
		okButton = new QPushButton("OK",this);
		cancelButton = new QPushButton("Cancel",this);
		projectID = -1;
		bimserver::BIMserverConnection *m_bimserverConnector = bc;

		connect(okButton, SIGNAL(clicked()),this, SLOT(okButton_clicked()));
		connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

		mainLayout->addWidget(introLabel,0,0,1,2);
		mainLayout->addWidget(listWidget,1,0,1,2);    	
		mainLayout->addWidget(okButton,2,0,1,1);
		mainLayout->addWidget(cancelButton,2,1,1,1);
		setLayout(mainLayout);
		setWindowTitle("Import Project");
		
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
		QMessageBox::information(this, "ProjectImportation", "projectList received, start to show windows");
		foreach(QString itm, pList)
		{
			listWidget->addItem(itm);
			QMessageBox::information(this, "ProjectImportation", itm.toStdString().c_str());
		}
		listWidget->update();
		this->update();
		QMessageBox::information(this, "ProjectImportation", "project list is added and updated.");
	}

	void ProjectImportation::okButton_clicked()
	{
		// (bug fix) if nothing is selected. 
		projectID = listWidget->currentItem()->text().section(":",0,0).toInt();
		m_bimserverConnector->download(projectID); 
	}

} // bimserver
} // openstudio
