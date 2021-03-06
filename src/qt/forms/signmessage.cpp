#include "signmessage.h"
#include "ui_signmessage.h"
#include "addressbookpage.h"
#include "base58.h"
#include "guiutil.h"
#include "init.h"
#include "main.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "wallet.h"

#include <QClipboard>

#include <string>
#include <vector>
SignMessage::SignMessage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SignMessage)
{
    ui->setupUi(this);
#if (QT_VERSION >= 0x040700)
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    ui->addressIn_SM->setPlaceholderText(tr("Enter a Shard address (e.g. SNZ3SxusyhLVUGc4uQJiGcBTrtBbyQbLJV)"));
    ui->signatureOut_SM->setPlaceholderText(tr("Click \"Sign Message\" to generate signature"));

#endif

    GUIUtil::setupAddressWidget(ui->addressIn_SM, this);

    ui->addressIn_SM->installEventFilter(this);
    ui->messageIn_SM->installEventFilter(this);
    ui->signatureOut_SM->installEventFilter(this);


    ui->signatureOut_SM->setFont(GUIUtil::bitcoinAddressFont());

    //connect(ui->addressBookButton_SM,SIGNAL(clicked()),this,SLOT(on_addressBookButton_SM_clicked()));
    connect(ui->pasteButton_SM,SIGNAL(clicked()),this,SLOT(on_pasteButton_SM_clicked()));
    connect(ui->signMessageButton_SM,SIGNAL(clicked()),this,SLOT(on_signMessageButton_SM_clicked()));
    connect(ui->copySignatureButton_SM,SIGNAL(clicked()),this,SLOT(on_copySignatureButton_SM_clicked()));
    connect(ui->clearButton_SM,SIGNAL(clicked()),this,SLOT(on_clearButton_SM_clicked()));
}




SignMessage::~SignMessage()
{
    delete ui;
}
void SignMessage::setModel(WalletModel *model)
{
    this->model = model;
}

void SignMessage::setAddress_SM(QString address)
{
    ui->addressIn_SM->setText(address);
    ui->messageIn_SM->setFocus();
}


void SignMessage::on_addressBookButton_SM_clicked()
{
    if (model && model->getAddressTableModel())
    {
        AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::ReceivingTab, this);
        dlg.setModel(model->getAddressTableModel());
        if (dlg.exec())
        {
            setAddress_SM(dlg.getReturnValue());
        }
    }
}

void SignMessage::on_pasteButton_SM_clicked()
{
    setAddress_SM(QApplication::clipboard()->text());
}

void SignMessage::on_signMessageButton_SM_clicked()
{
    if (!model)
        return;

    /* Clear old signature to ensure users don't get confused on error with an old signature displayed */
    ui->signatureOut_SM->clear();

    CBitcoinAddress addr(ui->addressIn_SM->text().toStdString());
    if (!addr.IsValid())
    {
        ui->addressIn_SM->setValid(false);
        ui->statusLabel_SM->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SM->setText(tr("The entered address is invalid.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }
    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
    {
        ui->addressIn_SM->setValid(false);
        ui->statusLabel_SM->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SM->setText(tr("The entered address does not refer to a key.") + QString(" ") + tr("Please check the address and try again."));
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if (!ctx.isValid())
    {
        ui->statusLabel_SM->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SM->setText(tr("Wallet unlock was cancelled."));
        return;
    }

    CKey key;
    if (!pwalletMain->GetKey(keyID, key))
    {
        ui->statusLabel_SM->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SM->setText(tr("Private key for the entered address is not available."));
        return;
    }

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << ui->messageIn_SM->document()->toPlainText().toStdString();

    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(Hash(ss.begin(), ss.end()), vchSig))
    {
        ui->statusLabel_SM->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel_SM->setText(QString("<nobr>") + tr("Message signing failed.") + QString("</nobr>"));
        return;
    }

    ui->statusLabel_SM->setStyleSheet("QLabel { color: green; }");
    ui->statusLabel_SM->setText(QString("<nobr>") + tr("Message signed.") + QString("</nobr>"));

    ui->signatureOut_SM->setText(QString::fromStdString(EncodeBase64(&vchSig[0], vchSig.size())));
}

void SignMessage::on_copySignatureButton_SM_clicked()
{
    QApplication::clipboard()->setText(ui->signatureOut_SM->text());
}

void SignMessage::on_clearButton_SM_clicked()
{
    ui->addressIn_SM->clear();
    ui->messageIn_SM->clear();
    ui->signatureOut_SM->clear();
    ui->statusLabel_SM->clear();

    ui->addressIn_SM->setFocus();
}




