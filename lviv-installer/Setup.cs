// GTA LVIV BETA installer / uninstaller (.NET Framework 4.8, WinForms, runs elevated).
// Build: lviv-installer\build-setup.ps1. Payload: Release\GTA LVIV BETA as embedded payload.zip.
// UNINSTALLER define builds the small Uninstall.exe without payload.
using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Win32;

static class Brand
{
    public const string Name = "GTA LVIV BETA";
    public const string Exe = "GTA LVIV BETA.exe";
    public const string RegKey = @"SOFTWARE\GTA LVIV BETA";
    public const string UninstallKey = @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\GTA LVIV BETA";
    public const string Version = "1.6.0";
    public static string DataDir { get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), Name); } }
    public static readonly Color Bg = Color.FromArgb(6, 13, 25), Panel = Color.FromArgb(12, 26, 46), Accent = Color.FromArgb(29, 155, 255), Gold = Color.FromArgb(255, 214, 78);
}

static class Machine
{
    static NTAccount Users { get { return (NTAccount)new SecurityIdentifier(WellKnownSidType.BuiltinUsersSid, null).Translate(typeof(NTAccount)); } }

    // Same registration as lviv-installer\register.ps1 (what the MTA 1.6 loader expects)
    public static void Register(string installDir)
    {
        using (var hklm = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32))
        using (var key = hklm.CreateSubKey(Brand.RegKey))
        {
            var acl = key.GetAccessControl();
            acl.AddAccessRule(new RegistryAccessRule(Users, RegistryRights.FullControl, InheritanceFlags.ContainerInherit, PropagationFlags.None, AccessControlType.Allow));
            key.SetAccessControl(acl);
            using (var ver = key.CreateSubKey("1.6")) ver.SetValue("Last Install Location", installDir);
            key.CreateSubKey("Common").Close();
        }
        foreach (var dir in new[] { Brand.DataDir, Path.Combine(Brand.DataDir, "Common"), Path.Combine(Brand.DataDir, "1.6") })
            Directory.CreateDirectory(dir);
        GrantModify(Brand.DataDir);
        GrantModify(installDir);  // the client writes config/logs next to itself, like official MTA
    }

    static void GrantModify(string dir)
    {
        var di = new DirectoryInfo(dir);
        var acl = di.GetAccessControl();
        acl.AddAccessRule(new FileSystemAccessRule(Users, FileSystemRights.Modify, InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit, PropagationFlags.None, AccessControlType.Allow));
        di.SetAccessControl(acl);
    }

    public static void WriteUninstallEntry(string installDir)
    {
        using (var hklm = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32))
        using (var key = hklm.CreateSubKey(Brand.UninstallKey))
        {
            key.SetValue("DisplayName", Brand.Name);
            key.SetValue("DisplayVersion", Brand.Version);
            key.SetValue("Publisher", "LVIV GTA");
            key.SetValue("DisplayIcon", Path.Combine(installDir, Brand.Exe));
            key.SetValue("InstallLocation", installDir);
            key.SetValue("UninstallString", "\"" + Path.Combine(installDir, "Uninstall.exe") + "\"");
            key.SetValue("NoModify", 1, RegistryValueKind.DWord);
            key.SetValue("NoRepair", 1, RegistryValueKind.DWord);
        }
    }

    public static string InstalledDir()
    {
        using (var hklm = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32))
        using (var key = hklm.OpenSubKey(Brand.RegKey + @"\1.6"))
            return key == null ? null : key.GetValue("Last Install Location") as string;
    }

    public static void Unregister()
    {
        using (var hklm = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32))
        {
            hklm.DeleteSubKeyTree(Brand.RegKey, false);
            hklm.DeleteSubKeyTree(Brand.UninstallKey, false);
        }
    }

    public static void Shortcut(string lnk, string target, string workDir)
    {
        Type t = Type.GetTypeFromProgID("WScript.Shell");
        dynamic shell = Activator.CreateInstance(t);
        dynamic sc = shell.CreateShortcut(lnk);
        sc.TargetPath = target;
        sc.WorkingDirectory = workDir;
        sc.IconLocation = target + ",0";
        sc.Description = Brand.Name;
        sc.Save();
    }

    public static string DesktopLnk { get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonDesktopDirectory), Brand.Name + ".lnk"); } }
    public static string StartMenuDir { get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonPrograms), Brand.Name); } }
}

class SetupForm : Form
{
    readonly TextBox pathBox = new TextBox();
    readonly CheckBox chkDesktop = new CheckBox(), chkLaunch = new CheckBox(), chkAgree = new CheckBox();
    readonly ProgressBar bar = new ProgressBar();
    readonly Label status = new Label();
    readonly Button btnInstall = new Button(), btnBrowse = new Button();
    bool done;

    static Button Styled(Button b, string text, bool primary)
    {
        b.Text = text; b.FlatStyle = FlatStyle.Flat; b.ForeColor = Color.White; b.Cursor = Cursors.Hand;
        b.BackColor = primary ? Brand.Accent : Brand.Panel; b.FlatAppearance.BorderColor = primary ? Color.FromArgb(54, 181, 255) : Color.FromArgb(60, 90, 120);
        b.Font = new Font("Segoe UI", 10f, FontStyle.Bold);
        return b;
    }

    public SetupForm()
    {
        Text = Brand.Name + " — встановлення";
        FormBorderStyle = FormBorderStyle.FixedDialog; MaximizeBox = false; StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(720, 460); BackColor = Brand.Bg; ForeColor = Color.White; Font = new Font("Segoe UI", 9.5f);
        try { Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); } catch { }

        var logo = new PictureBox { Location = new Point(24, 24), Size = new Size(190, 250), SizeMode = PictureBoxSizeMode.Zoom, BackColor = Color.Transparent };
        using (var s = Assembly.GetExecutingAssembly().GetManifestResourceStream("logo.png")) if (s != null) logo.Image = Image.FromStream(s);
        Controls.Add(logo);

        Controls.Add(new Label { Text = "GTA LVIV BETA", Location = new Point(236, 24), AutoSize = true, Font = new Font("Segoe UI", 22f, FontStyle.Bold), ForeColor = Color.White });
        Controls.Add(new Label { Text = "Український RolePlay • клієнт на базі MTA:SA 1.6", Location = new Point(240, 66), AutoSize = true, ForeColor = Brand.Gold });
        Controls.Add(new Label
        {
            Location = new Point(240, 100), Size = new Size(450, 96), ForeColor = Color.FromArgb(180, 205, 225),
            Text = "Клієнт підключається лише до офіційного сервера проєкту.\r\n" +
                   "Потрібна легально встановлена GTA: San Andreas — її файли не входять до збірки.\r\n" +
                   "Інші встановлені копії MTA не змінюються.\r\n" +
                   "Збірка базується на Multi Theft Auto (GPLv3), вихідний код:\r\ngithub.com/WinstonBleyz/lvivgta-mta"
        });

        Controls.Add(new Label { Text = "Папка встановлення:", Location = new Point(240, 206), AutoSize = true });
        pathBox.Location = new Point(240, 228); pathBox.Size = new Size(350, 26); pathBox.BackColor = Brand.Panel; pathBox.ForeColor = Color.White; pathBox.BorderStyle = BorderStyle.FixedSingle;
        pathBox.Text = Machine.InstalledDir() ?? @"C:\Games\GTA LVIV BETA";
        Controls.Add(pathBox);
        Styled(btnBrowse, "Огляд…", false); btnBrowse.Location = new Point(598, 226); btnBrowse.Size = new Size(94, 28);
        btnBrowse.Click += (s, e) => { using (var d = new FolderBrowserDialog { SelectedPath = pathBox.Text }) if (d.ShowDialog(this) == DialogResult.OK) pathBox.Text = Path.Combine(d.SelectedPath, Brand.Name); };
        Controls.Add(btnBrowse);

        chkDesktop.Text = "Ярлик на робочому столі"; chkDesktop.Checked = true; chkDesktop.Location = new Point(240, 266); chkDesktop.AutoSize = true;
        chkLaunch.Text = "Запустити після встановлення"; chkLaunch.Checked = true; chkLaunch.Location = new Point(240, 290); chkLaunch.AutoSize = true;
        chkAgree.Text = "Я приймаю ліцензію GPLv3 (LICENSE.txt) і маю легальну копію GTA: San Andreas"; chkAgree.Location = new Point(240, 314); chkAgree.Size = new Size(460, 40);
        chkAgree.CheckedChanged += (s, e) => btnInstall.Enabled = chkAgree.Checked && !done;
        Controls.AddRange(new Control[] { chkDesktop, chkLaunch, chkAgree });

        bar.Location = new Point(24, 372); bar.Size = new Size(668, 10); Controls.Add(bar);
        status.Location = new Point(24, 390); status.Size = new Size(500, 40); status.ForeColor = Color.FromArgb(150, 175, 195); Controls.Add(status);
        Styled(btnInstall, "ВСТАНОВИТИ", true); btnInstall.Location = new Point(542, 396); btnInstall.Size = new Size(150, 40); btnInstall.Enabled = false;
        btnInstall.Click += async (s, e) => await InstallAsync();
        Controls.Add(btnInstall);
    }

    async Task InstallAsync()
    {
        if (done) { Close(); return; }
        string dir = pathBox.Text.Trim();
        if (dir.Length < 4 || !Path.IsPathRooted(dir)) { MessageBox.Show(this, "Вкажіть повний шлях до папки.", Brand.Name); return; }
        if (Process.GetProcessesByName("GTA LVIV BETA").Length > 0 || Process.GetProcessesByName("gta_sa").Length > 0)
        { MessageBox.Show(this, "Закрийте гру перед встановленням.", Brand.Name); return; }

        btnInstall.Enabled = btnBrowse.Enabled = pathBox.Enabled = chkAgree.Enabled = false;
        var progress = new Progress<Tuple<int, string>>(p => { bar.Value = p.Item1; status.Text = p.Item2; });
        try
        {
            bool desktop = chkDesktop.Checked;
            await Task.Run(() => Install(dir, desktop, progress));
            done = true;
            status.Text = "Готово! GTA LVIV BETA встановлено в " + dir;
            btnInstall.Text = "ЗАКРИТИ"; btnInstall.Enabled = true;
            if (chkLaunch.Checked)
                Process.Start(new ProcessStartInfo(Path.Combine(dir, Brand.Exe)) { WorkingDirectory = dir, UseShellExecute = true });
        }
        catch (Exception ex)
        {
            status.Text = "Помилка: " + ex.Message;
            MessageBox.Show(this, "Встановлення не вдалося:\r\n" + ex.Message, Brand.Name, MessageBoxButtons.OK, MessageBoxIcon.Error);
            btnInstall.Enabled = btnBrowse.Enabled = pathBox.Enabled = chkAgree.Enabled = true;
        }
    }

    static void Install(string dir, bool desktop, IProgress<Tuple<int, string>> progress)
    {
        Directory.CreateDirectory(dir);
        using (var s = Assembly.GetExecutingAssembly().GetManifestResourceStream("payload.zip"))
        using (var zip = new ZipArchive(s, ZipArchiveMode.Read))
        {
            int i = 0, n = zip.Entries.Count;
            string root = Path.GetFullPath(dir) + Path.DirectorySeparatorChar;
            foreach (var entry in zip.Entries)
            {
                string dest = Path.GetFullPath(Path.Combine(dir, entry.FullName));
                if (!dest.StartsWith(root, StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("bad path " + entry.FullName);
                if (entry.FullName.EndsWith("/")) Directory.CreateDirectory(dest);
                else { Directory.CreateDirectory(Path.GetDirectoryName(dest)); entry.ExtractToFile(dest, true); }
                if (++i % 25 == 0 || i == n) progress.Report(Tuple.Create(i * 85 / n, "Копіювання файлів… " + i + " / " + n));
            }
        }

        progress.Report(Tuple.Create(88, "Реєстрація клієнта…"));
        using (var s = Assembly.GetExecutingAssembly().GetManifestResourceStream("Uninstall.exe"))
        using (var f = File.Create(Path.Combine(dir, "Uninstall.exe"))) s.CopyTo(f);
        Machine.Register(dir);
        Machine.WriteUninstallEntry(dir);

        progress.Report(Tuple.Create(95, "Створення ярликів…"));
        string exe = Path.Combine(dir, Brand.Exe);
        Directory.CreateDirectory(Machine.StartMenuDir);
        Machine.Shortcut(Path.Combine(Machine.StartMenuDir, Brand.Name + ".lnk"), exe, dir);
        Machine.Shortcut(Path.Combine(Machine.StartMenuDir, "Видалити " + Brand.Name + ".lnk"), Path.Combine(dir, "Uninstall.exe"), dir);
        if (desktop) Machine.Shortcut(Machine.DesktopLnk, exe, dir);
        progress.Report(Tuple.Create(100, "Готово"));
    }
}

static class Program
{
    [STAThread]
    static int Main(string[] args)
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
#if UNINSTALLER
        return Uninstall(Array.Exists(args, a => a.Equals("/S", StringComparison.OrdinalIgnoreCase)));
#else
        Application.Run(new SetupForm());
        return 0;
#endif
    }

#if UNINSTALLER
    static int Uninstall(bool silent)
    {
        string dir = Path.GetDirectoryName(Application.ExecutablePath);
        if (!silent && MessageBox.Show("Видалити " + Brand.Name + " з цього комп'ютера?\r\nGTA: San Andreas і інші копії MTA не зачіпаються.",
                Brand.Name, MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return 1;
        if (Process.GetProcessesByName("GTA LVIV BETA").Length > 0) { MessageBox.Show("Спершу закрийте гру.", Brand.Name); return 2; }

        foreach (var p in new[] { Machine.DesktopLnk }) try { File.Delete(p); } catch { }
        try { Directory.Delete(Machine.StartMenuDir, true); } catch { }
        Machine.Unregister();
        try { Directory.Delete(Brand.DataDir, true); } catch { }

        // Delete the install folder after this process exits
        string cmd = "/c timeout /t 2 /nobreak >nul & rmdir /s /q \"" + dir + "\"";
        Process.Start(new ProcessStartInfo("cmd.exe", cmd) { CreateNoWindow = true, UseShellExecute = false, WorkingDirectory = Path.GetTempPath() });
        if (!silent) MessageBox.Show(Brand.Name + " видалено.", Brand.Name);
        return 0;
    }
#endif
}
