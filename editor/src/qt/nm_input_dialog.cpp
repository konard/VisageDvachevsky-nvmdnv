#include "NovelMind/editor/qt/nm_dialogs.hpp"
#include "nm_dialogs_detail.hpp"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace NovelMind::editor::qt {

NMInputDialog::NMInputDialog(QWidget *parent, const QString &title,
                             const QString &label, InputType type)
    : QDialog(parent), m_type(type) {
  setWindowTitle(title);
  setModal(true);
  setObjectName("NMInputDialog");
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(8);

  m_label = new QLabel(label, this);
  m_label->setWordWrap(true);
  layout->addWidget(m_label);

  if (type == InputType::Text) {
    m_textEdit = new QLineEdit(this);
    layout->addWidget(m_textEdit);
  } else if (type == InputType::Int) {
    m_intSpin = new QSpinBox(this);
    layout->addWidget(m_intSpin);
  } else {
    m_doubleSpin = new QDoubleSpinBox(this);
    layout->addWidget(m_doubleSpin);
  }

  auto *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();

  m_okButton = new QPushButton(tr("OK"), this);
  m_okButton->setObjectName("NMPrimaryButton");
  m_okButton->setDefault(true);

  m_cancelButton = new QPushButton(tr("Cancel"), this);
  m_cancelButton->setObjectName("NMSecondaryButton");

  connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
  connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

  buttonLayout->addWidget(m_cancelButton);
  buttonLayout->addWidget(m_okButton);

  layout->addLayout(buttonLayout);

  detail::applyDialogFrameStyle(this);
  detail::animateDialogIn(this);
}

void NMInputDialog::configureText(const QString &text,
                                  QLineEdit::EchoMode mode) {
  if (m_textEdit) {
    m_textEdit->setEchoMode(mode);
    m_textEdit->setText(text);
    m_textEdit->selectAll();
    m_textEdit->setFocus();
  }
}

void NMInputDialog::configureInt(int value, int minValue, int maxValue,
                                 int step) {
  if (m_intSpin) {
    m_intSpin->setRange(minValue, maxValue);
    m_intSpin->setSingleStep(step);
    m_intSpin->setValue(value);
    m_intSpin->setFocus();
  }
}

void NMInputDialog::configureDouble(double value, double minValue,
                                    double maxValue, int decimals) {
  if (m_doubleSpin) {
    m_doubleSpin->setRange(minValue, maxValue);
    m_doubleSpin->setDecimals(decimals);
    m_doubleSpin->setValue(value);
    m_doubleSpin->setFocus();
  }
}

QString NMInputDialog::textValue() const {
  return m_textEdit ? m_textEdit->text() : QString();
}

int NMInputDialog::intValue() const { return m_intSpin ? m_intSpin->value() : 0; }

double NMInputDialog::doubleValue() const {
  return m_doubleSpin ? m_doubleSpin->value() : 0.0;
}

QString NMInputDialog::getText(QWidget *parent, const QString &title,
                               const QString &label,
                               QLineEdit::EchoMode mode,
                               const QString &text, bool *ok) {
  NMInputDialog dialog(parent, title, label, InputType::Text);
  dialog.configureText(text, mode);
  const int result = dialog.exec();
  if (ok) {
    *ok = (result == QDialog::Accepted);
  }
  return result == QDialog::Accepted ? dialog.textValue() : QString();
}

int NMInputDialog::getInt(QWidget *parent, const QString &title,
                          const QString &label, int value, int minValue,
                          int maxValue, int step, bool *ok) {
  NMInputDialog dialog(parent, title, label, InputType::Int);
  dialog.configureInt(value, minValue, maxValue, step);
  const int result = dialog.exec();
  if (ok) {
    *ok = (result == QDialog::Accepted);
  }
  return result == QDialog::Accepted ? dialog.intValue() : value;
}

double NMInputDialog::getDouble(QWidget *parent, const QString &title,
                                const QString &label, double value,
                                double minValue, double maxValue, int decimals,
                                bool *ok) {
  NMInputDialog dialog(parent, title, label, InputType::Double);
  dialog.configureDouble(value, minValue, maxValue, decimals);
  const int result = dialog.exec();
  if (ok) {
    *ok = (result == QDialog::Accepted);
  }
  return result == QDialog::Accepted ? dialog.doubleValue() : value;
}


} // namespace NovelMind::editor::qt
