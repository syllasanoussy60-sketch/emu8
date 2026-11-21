
// emu8guiDlg.cpp : fichier d'implémentation
//

#include "pch.h"
#include "framework.h"
#include "emu8gui.h"
#include "emu8guiDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
// Tables d'identifiants : on ne suppose jamais que les ID se suivent.
static const UINT ID_LED[8] = {
	IDC_LED0, IDC_LED1, IDC_LED2, IDC_LED3,
	IDC_LED4, IDC_LED5, IDC_LED6, IDC_LED7
};

static const UINT ID_SW[8] = {
	IDC_SW0, IDC_SW1, IDC_SW2, IDC_SW3,
	IDC_SW4, IDC_SW5, IDC_SW6, IDC_SW7
};

static const UINT_PTR TIMER_EXECUTION = 1;

// Renvoie le dossier de l'executable, termine par un backslash.
// Permet de trouver les programmes sans chemin code en dur.
static CString DossierExecutable()
{
	TCHAR chemin[MAX_PATH] = { 0 };
	GetModuleFileName(nullptr, chemin, MAX_PATH);

	CString s(chemin);
	const int p = s.ReverseFind(_T('\\'));
	return (p >= 0) ? s.Left(p + 1) : CString();
}




// boîte de dialogue CAboutDlg utilisée pour la boîte de dialogue 'À propos de' pour votre application

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Données de boîte de dialogue
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge de DDX/DDV

// Implémentation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// boîte de dialogue de Cemu8guiDlg



Cemu8guiDlg::Cemu8guiDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_EMU8GUI_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cemu8guiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cemu8guiDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CHARGER, &Cemu8guiDlg::OnBnClickedCharger)
	ON_BN_CLICKED(IDC_PAS, &Cemu8guiDlg::OnBnClickedPas)
	ON_BN_CLICKED(IDC_RUN, &Cemu8guiDlg::OnBnClickedRun)
	ON_BN_CLICKED(IDC_STOP, &Cemu8guiDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_RESET, &Cemu8guiDlg::OnBnClickedReset)
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
	ON_LBN_DBLCLK(IDC_LISTE_CODE, &Cemu8guiDlg::OnDblclkListeCode)
END_MESSAGE_MAP()

// gestionnaires de messages de Cemu8guiDlg

BOOL Cemu8guiDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Ajouter l'élément de menu "À propos de..." au menu Système.

	// IDM_ABOUTBOX doit se trouver dans la plage des commandes système.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Définir l'icône de cette boîte de dialogue.  L'infrastructure effectue cela automatiquement
	//  lorsque la fenêtre principale de l'application n'est pas une boîte de dialogue
	SetIcon(m_hIcon, TRUE);			// Définir une grande icône
	SetIcon(m_hIcon, FALSE);		// Définir une petite icône

	// TODO: ajoutez ici une initialisation supplémentaire
	  // ---- police a chasse fixe : les colonnes s'alignent ----
	m_police.CreatePointFont(100, _T("Consolas"));
	GetDlgItem(IDC_LISTE_CODE)->SetFont(&m_police);
	GetDlgItem(IDC_ETAT)->SetFont(&m_police);
	GetDlgItem(IDC_MEMOIRE)->SetFont(&m_police);

	// ---- couleurs des LED ----
	m_brosseAllumee.CreateSolidBrush(RGB(255, 40, 40));
	m_brosseEteinte.CreateSolidBrush(RGB(70, 70, 70));

	// ---- curseur de vitesse : nombre d'instructions par impulsion ----
	CSliderCtrl* pVitesse = (CSliderCtrl*)GetDlgItem(IDC_VITESSE);
	pVitesse->SetRange(1, 200);
	pVitesse->SetPos(20);

	m_enMarche = false;
	m_machine.reset();
	// Programme charge par defaut, pour que la fenetre ne soit pas vide.
	const CString defaut = DossierExecutable() + _T("programmes\\chenillard.hex");
	CStringA cheminDefaut(defaut);
	m_machine.charger(std::string(cheminDefaut));
	MettreAJourAffichage();


	return TRUE;  // retourne TRUE, sauf si vous avez défini le focus sur un contrôle
}

void Cemu8guiDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// Si vous ajoutez un bouton Réduire à votre boîte de dialogue, vous devez utiliser le code ci-dessous
//  pour dessiner l'icône.  Pour les applications MFC utilisant le modèle Document/Vue,
//  cela est fait automatiquement par l'infrastructure.

void Cemu8guiDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // contexte de périphérique pour la peinture

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Centrer l'icône dans le rectangle client
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Dessiner l'icône
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// Le système appelle cette fonction pour obtenir le curseur à afficher lorsque l'utilisateur fait glisser
//  la fenêtre réduite.
HCURSOR Cemu8guiDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
// =====================================================================
//  Affichage
//
//  L'interface se contente de LIRE l'etat de CMachine. Elle ne calcule
//  rien : listingAutourDuPc() et dumpMemoire() renvoient deja des
//  chaines pretes a afficher. C'est ce qui permet au coeur de rester
//  independant de MFC.
// =====================================================================

void Cemu8guiDlg::MettreAJourAffichage()
{
	const CProcesseur& cpu = m_machine.cpu();

	// ---- code desassemble ----
	CListBox* pListe = (CListBox*)GetDlgItem(IDC_LISTE_CODE);
	pListe->ResetContent();

	std::vector<std::string> lignes = m_machine.listingAutourDuPc(16);
	for (size_t i = 0; i < lignes.size(); ++i)
	{
		pListe->AddString(CString(lignes[i].c_str()));
	}
	if (!lignes.empty())
	{
		pListe->SetCurSel(0);   // la ligne courante est toujours la premiere
	}

	// ---- etat du processeur ----
	CString drapeaux(cpu.texteDrapeaux().c_str());
	CString statut(texteEtat(m_machine.dernierEtat()));

	CString etat;
	etat.Format(
		_T("R0=%02X  R1=%02X  R2=%02X  R3=%02X\r\n")
		_T("R4=%02X  R5=%02X  R6=%02X  R7=%02X\r\n")
		_T("\r\n")
		_T("PC=0x%02X   SP=0x%02X\r\n")
		_T("%s\r\n")
		_T("cycles=%llu   instructions=%llu\r\n")
		_T("\r\n")
		_T("%s"),
		cpu.registre(0), cpu.registre(1), cpu.registre(2), cpu.registre(3),
		cpu.registre(4), cpu.registre(5), cpu.registre(6), cpu.registre(7),
		cpu.pc(), cpu.sp(),
		(LPCTSTR)drapeaux,
		(unsigned long long)cpu.cycles(),
		(unsigned long long)cpu.nbInstructions(),
		(LPCTSTR)statut);
	SetDlgItemText(IDC_ETAT, etat);

	// ---- dump memoire ----
	CString memoire;
	std::vector<std::string> dump = m_machine.dumpMemoire(0x90, 0xFF);
	for (size_t i = 0; i < dump.size(); ++i)
	{
		memoire += CString(dump[i].c_str());
		memoire += _T("\r\n");
	}
	SetDlgItemText(IDC_MEMOIRE, memoire);

	// ---- LED : on force leur redessin, OnCtlColor fera la couleur ----
	for (int i = 0; i < 8; ++i)
	{
		GetDlgItem(ID_LED[i])->Invalidate();
	}
}

// =====================================================================
//  Lecture des interrupteurs
//
//  Appelee avant chaque instruction : le programme emule lit le port
//  0x81 au moment ou il execute son LD, exactement comme un vrai
//  microcontroleur echantillonne ses entrees.
// =====================================================================
void Cemu8guiDlg::LireInterrupteurs()
{
	uint8_t valeur = 0;
	for (int i = 0; i < 8; ++i)
	{
		if (IsDlgButtonChecked(ID_SW[i]))
		{
			valeur = (uint8_t)(valeur | (1u << i));
		}
	}
	m_machine.portEntree().setValeur(valeur);
}

// =====================================================================
//  Boutons
// =====================================================================

void Cemu8guiDlg::OnBnClickedPas()
{
	LireInterrupteurs();
	m_machine.pas();
	MettreAJourAffichage();
}

void Cemu8guiDlg::OnBnClickedRun()
{
	if (m_enMarche)
	{
		return;
	}
	m_enMarche = true;
	SetTimer(TIMER_EXECUTION, 50, nullptr);
}

void Cemu8guiDlg::OnBnClickedStop()
{
	if (!m_enMarche)
	{
		return;
	}
	KillTimer(TIMER_EXECUTION);
	m_enMarche = false;
}

void Cemu8guiDlg::OnBnClickedReset()
{
	OnBnClickedStop();
	m_machine.reset();
	MettreAJourAffichage();
}

void Cemu8guiDlg::OnBnClickedCharger()
{
	OnBnClickedStop();

	CFileDialog dlg(TRUE, _T("hex"), nullptr,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("Programmes (*.hex;*.bin)|*.hex;*.bin|Tous les fichiers (*.*)|*.*||"),
		this);
	const CString dossier = DossierExecutable() + _T("programmes");
	dlg.m_ofn.lpstrInitialDir = (LPCTSTR)dossier;
	if (dlg.DoModal() != IDOK)
	{
		return;
	}

	CStringA chemin(dlg.GetPathName());
	if (!m_machine.charger(std::string(chemin)))
	{
		AfxMessageBox(CString(m_machine.derniereErreur().c_str()));
		return;
	}

	m_machine.effacerPointsArret();
	MettreAJourAffichage();
}

// =====================================================================
//  Execution continue
//
//  Le timer donne la main a l'interface entre deux paquets
//  d'instructions : la fenetre reste reactive et on peut cliquer sur
//  Arreter. Le curseur de vitesse regle le nombre d'instructions
//  executees par impulsion.
// =====================================================================
void Cemu8guiDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_EXECUTION)
	{
		CSliderCtrl* pVitesse = (CSliderCtrl*)GetDlgItem(IDC_VITESSE);
		int nbPas = pVitesse->GetPos();
		if (nbPas < 1)
		{
			nbPas = 1;
		}

		LireInterrupteurs();

		for (int i = 0; i < nbPas; ++i)
		{
			const Etat e = m_machine.pas();

			if (e != Etat::OK)
			{
				OnBnClickedStop();        // HALT ou erreur d'execution
				MettreAJourAffichage();
				return;
			}
			if (m_machine.estPointArret(m_machine.cpu().pc()))
			{
				OnBnClickedStop();
				MettreAJourAffichage();
				return;
			}
		}
		MettreAJourAffichage();
	}

	CDialogEx::OnTimer(nIDEvent);
}

// =====================================================================
//  Points d'arret : double-clic dans la liste du code
//
//  Une ligne du listing a la forme "=> 0008  18 80   ST R0, [0x80]".
//  Les trois premiers caracteres sont le marqueur, les quatre suivants
//  l'adresse en hexadecimal.
// =====================================================================
void Cemu8guiDlg::OnDblclkListeCode()
{
	CListBox* pListe = (CListBox*)GetDlgItem(IDC_LISTE_CODE);

	const int selection = pListe->GetCurSel();
	if (selection == LB_ERR)
	{
		return;
	}

	CString ligne;
	pListe->GetText(selection, ligne);
	if (ligne.GetLength() < 7)
	{
		return;
	}

	const long adresse = _tcstol(ligne.Mid(3, 4), nullptr, 16);
	m_machine.basculePointArret((uint8_t)adresse);
	MettreAJourAffichage();
}

// =====================================================================
//  Couleur des LED
//
//  Appelee par Windows juste avant de dessiner chaque controle
//  statique. On ne repond que pour les huit LED ; tout le reste est
//  laisse au comportement par defaut.
// =====================================================================
HBRUSH Cemu8guiDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_STATIC && pWnd != nullptr)
	{
		const UINT identifiant = pWnd->GetDlgCtrlID();

		for (int i = 0; i < 8; ++i)
		{
			if (identifiant == ID_LED[i])
			{
				const bool allumee = m_machine.portSortie().led(i);

				pDC->SetBkColor(allumee ? RGB(255, 40, 40) : RGB(70, 70, 70));
				pDC->SetTextColor(RGB(255, 255, 255));

				CBrush& brosse = allumee ? m_brosseAllumee : m_brosseEteinte;
				return (HBRUSH)brosse;
			}
		}
	}

	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

