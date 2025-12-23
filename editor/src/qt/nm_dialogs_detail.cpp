#include "nm_dialogs_detail.hpp"

#include "NovelMind/editor/qt/nm_style_manager.hpp"

#include <QDialog>
#include <QPropertyAnimation>
#include <QTimer>

namespace NovelMind::editor::qt::detail {

void applyDialogFrameStyle(QDialog *dialog) {
  if (!dialog) {
    return;
  }
  const auto &p = NMStyleManager::instance().palette();
  dialog->setStyleSheet(QString(R"(
    QDialog {
      background-color: %1;
      border: 1px solid %2;
    }
    QLabel#NMMessageText {
      color: %3;
    }
    QPushButton#NMPrimaryButton {
      background-color: %4;
      color: %5;
      border: none;
      border-radius: 4px;
      padding: 5px 12px;
      font-weight: 600;
    }
    QPushButton#NMPrimaryButton:hover {
      background-color: %6;
    }
    QPushButton#NMSecondaryButton {
      background-color: %7;
      color: %5;
      border: 1px solid %8;
      border-radius: 4px;
      padding: 5px 12px;
    }
    QPushButton#NMSecondaryButton:hover {
      background-color: %9;
      border-color: %4;
    }
  )")
                             .arg(NMStyleManager::colorToStyleString(p.bgDark))
                             .arg(NMStyleManager::colorToStyleString(
                                 p.borderLight))
                             .arg(NMStyleManager::colorToStyleString(
                                 p.textPrimary))
                             .arg(NMStyleManager::colorToStyleString(
                                 p.accentPrimary))
                             .arg(NMStyleManager::colorToStyleString(
                                 p.textPrimary))
                             .arg(NMStyleManager::colorToStyleString(
                                 p.accentHover))
                             .arg(NMStyleManager::colorToStyleString(
                                 p.bgMedium))
                             .arg(NMStyleManager::colorToStyleString(
                                 p.borderLight))
                             .arg(NMStyleManager::colorToStyleString(
                                 p.bgLight)));
}

void animateDialogIn(QDialog *dialog) {
  if (!dialog) {
    return;
  }
  dialog->setWindowOpacity(0.0);
  QTimer::singleShot(0, dialog, [dialog]() {
    auto *anim = new QPropertyAnimation(dialog, "windowOpacity", dialog);
    anim->setDuration(160);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
  });
}

} // namespace NovelMind::editor::qt::detail
