#include "lal_dialog.h"
#include <QDebug>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>

LALDialog::LALDialog(QWidget *parent) : QDialog(parent) {
  setupUi();
  refreshTable();
}

LALDialog::~LALDialog() {}

void LALDialog::setupUi() {
  setWindowTitle(tr("Licensed Asset Loader (LAL) Beta"));
  resize(800, 400);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // Info Label
  QLabel *infoLabel = new QLabel(tr("<b>Licensed Asset Loader</b><br>"
                                    "Manage proprietary drivers and firmware "
                                    "required for certain devices.<br>"
                                    "Assets are installed locally to your user "
                                    "directory (no Wine required)."),
                                 this);
  mainLayout->addWidget(infoLabel);

  // Table
  assetTable = new QTableWidget(this);
  assetTable->setColumnCount(5);
  assetTable->setHorizontalHeaderLabels(
      {tr("Asset"), tr("Vendor"), tr("Version"), tr("Status"), tr("Action")});
  assetTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  assetTable->horizontalHeader()->setSectionResizeMode(
      4, QHeaderView::ResizeToContents);
  mainLayout->addWidget(assetTable);

  // Buttons
  QHBoxLayout *btnLayout = new QHBoxLayout();
  refreshButton = new QPushButton(tr("Refresh"), this);
  connect(refreshButton, &QPushButton::clicked, this, &LALDialog::onRefresh);

  closeButton = new QPushButton(tr("Close"), this);
  connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

  btnLayout->addStretch();
  btnLayout->addWidget(refreshButton);
  btnLayout->addWidget(closeButton);
  mainLayout->addLayout(btnLayout);
}

void LALDialog::refreshTable() {
  auto &mgr = lal::LALManager::instance();
  // Load manifest if not loaded (prototype path)
  // TODO: In real app, load from standard location
  if (mgr.getAssetIds().empty()) {
    mgr.loadManifest("src/lal/lal_manifest.json"); // Dev path
  }

  std::vector<std::string> ids = mgr.getAssetIds();
  assetTable->setRowCount(ids.size());

  int row = 0;
  for (const auto &id : ids) {
    auto def = mgr.getAssetDefinition(id);
    auto status = mgr.getAssetStatus(id);

    assetTable->setItem(row, 0,
                        new QTableWidgetItem(QString::fromStdString(def.name)));
    assetTable->setItem(
        row, 1, new QTableWidgetItem(QString::fromStdString(def.vendor)));
    assetTable->setItem(
        row, 2, new QTableWidgetItem(QString::fromStdString(def.version)));
    assetTable->setItem(row, 3, new QTableWidgetItem(getStatusString(status)));

    QPushButton *actionBtn = new QPushButton(tr("Install"), this);
    actionBtn->setProperty("asset_id", QString::fromStdString(id));
    connect(actionBtn, &QPushButton::clicked, this,
            &LALDialog::onInstallClicked);

    if (status == lal::AssetStatus::INSTALLED) {
      actionBtn->setText(tr("Reinstall"));
    }

    assetTable->setCellWidget(row, 4, actionBtn);
    row++;
  }
}

QString LALDialog::getStatusString(lal::AssetStatus status) {
  switch (status) {
  case lal::AssetStatus::INSTALLED:
    return tr("Installed");
  case lal::AssetStatus::MISSING:
    return tr("Missing");
  case lal::AssetStatus::CORRUPT:
    return tr("Corrupt/Invalid");
  default:
    return tr("Unknown");
  }
}

void LALDialog::onRefresh() { refreshTable(); }

void LALDialog::onInstallClicked() {
  QPushButton *btn = qobject_cast<QPushButton *>(sender());
  if (!btn)
    return;

  QString assetId = btn->property("asset_id").toString();
  auto &mgr = lal::LALManager::instance();
  auto def = mgr.getAssetDefinition(assetId.toStdString());

  // For prototype, ask user for the file
  // Real LAL will handle downloads or instructions based on 'AssetSource'
  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Select Archive for %1").arg(QString::fromStdString(def.name)),
      QDir::homePath(), tr("Archives (*.zip *.7z *.exe *.tar.gz *.tgz)"));

  if (fileName.isEmpty())
    return;

  if (mgr.installAssetFromArchive(assetId.toStdString(),
                                  fileName.toStdString())) {
    QMessageBox::information(this, tr("Success"),
                             tr("Asset installed successfully!"));
    refreshTable();
  } else {
    QMessageBox::critical(
        this, tr("Error"),
        tr("Failed to install asset. Check console for details."));
  }
}
