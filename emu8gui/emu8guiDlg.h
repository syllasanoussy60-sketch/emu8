#include "machine.h"
// emu8guiDlg.h : fichier d'en-tête
//

#pragma once


// boîte de dialogue de Cemu8guiDlg
class Cemu8guiDlg : public CDialogEx
{
	// Construction
public:
	Cemu8guiDlg(CWnd* pParent = nullptr);	// constructeur standard

	// Données de boîte de dialogue
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EMU8GUI_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// Prise en charge de DDX/DDV


	// Implémentation
protected:
	HICON m_hIcon;

	// Fonctions générées de la table des messages
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void   OnBnClickedCharger();
	afx_msg void   OnBnClickedPas();
	afx_msg void   OnBnClickedRun();
	afx_msg void   OnBnClickedStop();
	afx_msg void   OnBnClickedReset();
	afx_msg void   OnTimer(UINT_PTR nIDEvent);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void   OnDblclkListeCode();

private:
	CMachine m_machine;
	CFont    m_police;
	CBrush   m_brosseAllumee;
	CBrush   m_brosseEteinte;
	bool     m_enMarche;

	void MettreAJourAffichage();
	void LireInterrupteurs();

};
