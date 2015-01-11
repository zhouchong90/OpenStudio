#ifndef BIMSERVER_PROJECTIMPORTATION_HPP
#define BIMSERVER_PROJECTIMPORTATION_HPP

#include "BIMserverAPI.hpp"

#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include "BIMserverConnection.hpp"

namespace openstudio {
namespace bimserver {

		/// This shows a input dialog to gather project id for import
		class BIMSERVER_API ProjectImportation: public QDialog
		{
				Q_OBJECT

		public:

				/// Default constructor
				ProjectImportation(QWidget *parent, bimserver::BIMserverConnection *bc);

				/// Virtual destructor
				~ProjectImportation();

		public slots:

			/// Takes projectList from BIMserverConnection and prints out projects
				void processProjectList(QStringList projectList);

			signals:

				/// start connection
				void startConnection(QString username, QString password);

				/// send project ID to bimserverConnection
				void projectIDSelected(QString projectID);

		private:

				QString 				m_projectID;
				QLabel      		*m_introLabel;
				QListWidget 		*m_listWidget;
				QPushButton 		*m_okButton;
				QPushButton 		*m_cancelButton;
				bimserver::BIMserverConnection *m_bimserverConnector;


		private slots:

		 /// What to do when the user clicks on the okButton
				void okButton_clicked();

		};

} // bimserver
} // openstudio

#endif // BIMSERVER_PROJECTIMPORTATION_HPP 
