#ifndef LAL_DIALOG_H
#define LAL_DIALOG_H

#include "lal_manager.h"
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

class LALDialog : public QDialog {
  Q_OBJECT

public:
  explicit LALDialog(QWidget *parent = nullptr);
  ~LALDialog();

private slots:
  void onInstallClicked();
  void onRefresh();

private:
  void setupUi();
  void refreshTable();

  QTableWidget *assetTable;
  QPushButton *refreshButton;
  QPushButton *closeButton;

  // Helper to get formatted status string
  QString getStatusString(lal::AssetStatus status);
};

#endif // LAL_DIALOG_H
