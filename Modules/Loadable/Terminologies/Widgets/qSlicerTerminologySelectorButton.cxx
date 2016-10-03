/*=========================================================================

  Library:   CTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

// Terminology includes
#include "qSlicerTerminologyNavigatorWidget.h"
#include "qSlicerTerminologySelectorDialog.h"
#include "qSlicerTerminologySelectorButton.h"

// Qt includes
#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStylePainter>

class qSlicerTerminologySelectorButtonPrivate
{
  Q_DECLARE_PUBLIC(qSlicerTerminologySelectorButton);
protected:
  qSlicerTerminologySelectorButton* const q_ptr;
public:
  qSlicerTerminologySelectorButtonPrivate(qSlicerTerminologySelectorButton& object);
  void init();
  void computeIcon();
  QString text()const;

  QIcon  Icon;
  vtkSlicerTerminologyEntry* TerminologyEntry;
  mutable QSize CachedSizeHint;
};

//-----------------------------------------------------------------------------
qSlicerTerminologySelectorButtonPrivate::qSlicerTerminologySelectorButtonPrivate(qSlicerTerminologySelectorButton& object)
  : q_ptr(&object)
{
  this->TerminologyEntry = NULL;
}

//-----------------------------------------------------------------------------
void qSlicerTerminologySelectorButtonPrivate::init()
{
  Q_Q(qSlicerTerminologySelectorButton);
  q->setCheckable(true);
  QObject::connect(q, SIGNAL(toggled(bool)),
                   q, SLOT(onToggled(bool)));
  this->computeIcon();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologySelectorButtonPrivate::computeIcon()
{
  Q_Q(qSlicerTerminologySelectorButton);

  // Get recommended color from terminology entry
  QColor recommendedColor = qSlicerTerminologyNavigatorWidget::recommendedColorFromTerminology(this->TerminologyEntry);

  int _iconSize = q->style()->pixelMetric(QStyle::PM_SmallIconSize);
  QPixmap pix(_iconSize, _iconSize);
  pix.fill(recommendedColor.isValid() ? q->palette().button().color() : Qt::transparent);
  QPainter p(&pix);
  p.setPen(QPen(Qt::gray));
  p.setBrush(recommendedColor.isValid() ? recommendedColor : QBrush(Qt::NoBrush));
  p.drawRect(2, 2, pix.width() - 5, pix.height() - 5);

  this->Icon = QIcon(pix);
}

//-----------------------------------------------------------------------------
qSlicerTerminologySelectorButton::qSlicerTerminologySelectorButton(QWidget* _parent)
  : QPushButton(_parent)
  , d_ptr(new qSlicerTerminologySelectorButtonPrivate(*this))
{
  Q_D(qSlicerTerminologySelectorButton);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerTerminologySelectorButton::~qSlicerTerminologySelectorButton()
{
}

//-----------------------------------------------------------------------------
void qSlicerTerminologySelectorButton::changeTerminology()
{
  Q_D(qSlicerTerminologySelectorButton);
  if (qSlicerTerminologySelectorDialog::getTerminology(d->TerminologyEntry, this))
    {
    d->computeIcon();
    this->update();
    emit terminologyChanged(d->TerminologyEntry);
    }
else
{
int i=0; ++i; //TODO: TEST
}
}

//-----------------------------------------------------------------------------
void qSlicerTerminologySelectorButton::onToggled(bool change)
{
  if (change)
    {
    this->changeTerminology();
    this->setChecked(false);
    }
}

//-----------------------------------------------------------------------------
void qSlicerTerminologySelectorButton::setTerminologyEntry(vtkSlicerTerminologyEntry* newTerminology)
{
  Q_D(qSlicerTerminologySelectorButton);
  if (newTerminology == d->TerminologyEntry)
    {
    return;
    }

  d->TerminologyEntry = newTerminology;
  d->computeIcon();

  this->update();
  emit terminologyChanged(d->TerminologyEntry);
}

//-----------------------------------------------------------------------------
vtkSlicerTerminologyEntry* qSlicerTerminologySelectorButton::terminologyEntry()
{
  Q_D(qSlicerTerminologySelectorButton);
  return d->TerminologyEntry;
}

//-----------------------------------------------------------------------------
void qSlicerTerminologySelectorButton::paintEvent(QPaintEvent *)
{
  Q_D(qSlicerTerminologySelectorButton);
  QStylePainter p(this);
  QStyleOptionButton option;
  this->initStyleOption(&option);
  //option.text = d->text();
  option.icon = d->Icon;
  p.drawControl(QStyle::CE_PushButton, option);
}

//-----------------------------------------------------------------------------
QSize qSlicerTerminologySelectorButton::sizeHint()const
{
  Q_D(const qSlicerTerminologySelectorButton);
//  if (!d->ShowTerminologyTypeName && !this->text().isEmpty())
//    {
    return this->QPushButton::sizeHint(); //TODO:
//    }
  if (d->CachedSizeHint.isValid())
    {
    return d->CachedSizeHint;
    }

  // If no text, the sizehint is a QToolButton sizeHint
  QStyleOptionButton pushButtonOpt;
  this->initStyleOption(&pushButtonOpt);
  //pushButtonOpt.text = d->text();
  int iconSize = this->style()->pixelMetric(QStyle::PM_SmallIconSize);
  if (pushButtonOpt.text == QString())
    {
    QStyleOptionToolButton opt;
    (&opt)->QStyleOption::operator=(pushButtonOpt);
    opt.arrowType = Qt::NoArrow;
    opt.icon = d->Icon;
    opt.iconSize = QSize(iconSize, iconSize);
    opt.rect.setSize(opt.iconSize); // PM_MenuButtonIndicator depends on the height
    d->CachedSizeHint = this->style()->sizeFromContents(
      QStyle::CT_ToolButton, &opt, opt.iconSize, this).
      expandedTo(QApplication::globalStrut());
    }
  else
    {
qCritical("ZZZ Non-empty text in button style. Cannot remove Private::text()"); //TODO:
    pushButtonOpt.icon = d->Icon;
    pushButtonOpt.iconSize = QSize(iconSize, iconSize);
    pushButtonOpt.rect.setSize(pushButtonOpt.iconSize); // PM_MenuButtonIndicator depends on the height
    d->CachedSizeHint = (style()->sizeFromContents(
                           QStyle::CT_PushButton, &pushButtonOpt, pushButtonOpt.iconSize, this).
                         expandedTo(QApplication::globalStrut()));
    }
  return d->CachedSizeHint;
}
