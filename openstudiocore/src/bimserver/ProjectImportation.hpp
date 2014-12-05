#ifndef BIMSERVER_PROJECTIMPORTATION_HPP
#define BIMSERVER_PROJECTIMPORTATION_HPP

#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include "BIMserverConnection.hpp"

namespace openstudio {
namespace bimserver {

    /// This shows a input dialog to gather project id for import
    class ProjectImportation: public QDialog
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

    private:

    	int 				projectID;
        QLabel      		*introLabel;
        QListWidget 		*listWidget;
        QPushButton 		*okButton;
        QPushButton 		*cancelButton;
        bimserver::BIMserverConnection *m_bimserverConnector;


    private slots:

	   /// What to do when the user clicks on the okButton
        void okButton_clicked();

    };

} // bimserver
} // openstudio

#endif // BIMSERVER_PROJECTIMPORTATION_HPP 
