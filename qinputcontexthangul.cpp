#include <ctype.h>

#include <qapplication.h>
#include <qstring.h>
#include <qevent.h>

#include "hangul.h"
#include "qinputcontexthangul.h"

HangulComposer::HangulComposer(hangul::ComposerBase::Keyboard keyboard, QInputContextHangul *context) :
    ComposerBase(keyboard),
    m_context(context)
{
}

QString HangulComposer::getPreeditQString(hangul::widestring &text)
{
    QChar ch(0);
    QString str;
    hangul::widestring::iterator iter;

    for (iter = text.begin(); iter != text.end(); ++iter) {
	ch = (uint)(*iter);
	str += ch;
    }
    return str;
}

void HangulComposer::preeditUpdate(hangul::widestring &text)
{
    QString preeditString = getPreeditQString(text);
    m_context->preeditUpdate(preeditString);
}

void HangulComposer::commit(hangul::widestring &text)
{
    QString preeditString = getPreeditQString(text);
    m_context->commit(preeditString);
}

QInputContextHangul::QInputContextHangul(hangul::ComposerBase::Keyboard keyboard) :
    m_candidateList(NULL),
    m_composer(keyboard, this),
    m_mode(MODE_DIRECT),
    m_rect(0, 0, 0, 0)
{
    qDebug("Hangul::");
}

QInputContextHangul::~QInputContextHangul()
{
    qDebug("Hangul::~");
}

QString QInputContextHangul::identifierName()
{
    return "Hangul";
}

QString QInputContextHangul::language()
{
    return "Hangul";
}

void QInputContextHangul::setFocus()
{
    if (m_mode == MODE_DIRECT) {
	setModeInfo(1);
    } else {
	setModeInfo(2);
    }

    qDebug("Hangul::setFocus");
}

void QInputContextHangul::unsetFocus()
{
    if (m_candidateList != NULL) {
	delete m_candidateList;
	m_candidateList = NULL;
	m_composer.reset();
    }
    m_composer.reset();

    setModeInfo(2);

    qDebug("Hangul::unsetFocus");
}

void QInputContextHangul::setMicroFocus(int x, int y, int w, int h, QFont* /*f*/)
{
    m_rect.setRect(x, y, w, h);
}

void QInputContextHangul::reset()
{
    m_composer.reset();
    qDebug("Hangul::reset");
}

void QInputContextHangul::preeditUpdate(const QString &preeditString)
{
    if (!isComposing()) {
	sendIMEvent(QEvent::IMStart);
    }
    sendIMEvent(QEvent::IMCompose, preeditString, preeditString.length());
}

void QInputContextHangul::commit(const QString &preeditString)
{
    if (!isComposing()) {
	sendIMEvent(QEvent::IMStart);
    }
    sendIMEvent(QEvent::IMEnd, preeditString);
}

bool QInputContextHangul::popupCandidateList()
{
    hangul::widestring text = m_composer.getPreeditString();
    m_candidateList = new CandidateList(text[0], m_rect.left(), m_rect.bottom());

    return true;
}

bool QInputContextHangul::filterEvent(const QEvent *event)
{
    if (event->type() == QEvent::KeyRelease)
	return false;

    const QKeyEvent *keyevent = static_cast<const QKeyEvent*>(event);
    if (m_candidateList != NULL) {
	bool ret = m_candidateList->filterEvent(keyevent);
	if (m_candidateList->isClosed()) {
	    if (m_candidateList->isSelected()) {
		m_composer.clear();
		QString candidate(m_candidateList->getCandidate());
		commit(candidate);
	    }
	    delete m_candidateList;
	    m_candidateList = NULL;
	}
	return ret;
    }

    if (keyevent->key() == Qt::Key_Shift)
	return false;

    if (keyevent->key() == Qt::Key_Backspace) {
	return m_composer.backspace();
    }

    if (keyevent->key() == Qt::Key_Space &&
	(keyevent->state() & Qt::ShiftButton) == Qt::ShiftButton) {
	int mode = 0;
	if (m_mode == MODE_DIRECT) {
	    m_mode = MODE_HANGUL;
	    mode = 2;
	} else {
	    m_composer.reset();
	    m_mode = MODE_DIRECT;
	    mode = 1;
	}
	setModeInfo(mode);

	return true;
    }

    if (keyevent->key() == Qt::Key_F9) {
	return popupCandidateList();
    }

    if (m_mode == MODE_HANGUL) {
	return m_composer.filter(keyevent->ascii());
    }

    return false;
}
