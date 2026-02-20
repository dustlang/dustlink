use std::env;
use std::ffi::OsString;
use std::path::{Path, PathBuf};
use std::process::Command;

const VERSION: &str = "0.1.0";

fn main() {
    if let Err(err) = run() {
        eprintln!("dustlink: {}", err);
        std::process::exit(1);
    }
}

fn run() -> Result<(), String> {
    let args: Vec<OsString> = env::args_os().skip(1).collect();

    if args.is_empty() || is_flag(&args[0], "-h") || is_flag(&args[0], "--help") {
        print_help();
        return Ok(());
    }
    if is_flag(&args[0], "-V") || is_flag(&args[0], "--version") {
        println!("dustlink {}", VERSION);
        return Ok(());
    }

    let backend = resolve_backend()?;
    if is_flag(&args[0], "--print-backend") {
        println!("{}", backend.display());
        return Ok(());
    }

    let mut forwarded: Vec<OsString> = Vec::new();
    if is_rust_lld(&backend) && !has_flavor_flag(&args) {
        forwarded.push(OsString::from("-flavor"));
        forwarded.push(OsString::from("gnu"));
    }
    forwarded.extend(args);

    let status = Command::new(&backend)
        .args(&forwarded)
        .status()
        .map_err(|e| format!("failed to invoke backend '{}': {}", backend.display(), e))?;

    match status.code() {
        Some(code) => std::process::exit(code),
        None => Err("backend terminated by signal".to_string()),
    }
}

fn print_help() {
    println!("dustlink {} - ld.lld-compatible frontend", VERSION);
    println!();
    println!("Usage:");
    println!("  dustlink [ld.lld-flags...]");
    println!();
    println!("Special flags:");
    println!("  --print-backend    Print resolved linker backend path");
    println!("  -h, --help         Show help");
    println!("  -V, --version      Show version");
    println!();
    println!("Backend resolution order:");
    println!("  1) DUSTLINK_BACKEND environment variable");
    println!("  2) ld.lld in PATH");
    println!("  3) rust-lld in PATH");
    println!("  4) rust-lld from rustc sysroot");
}

fn resolve_backend() -> Result<PathBuf, String> {
    if let Some(raw) = env::var_os("DUSTLINK_BACKEND") {
        let value = PathBuf::from(raw);
        if value.components().count() > 1 || value.is_absolute() {
            if value.is_file() {
                return Ok(value);
            }
            return Err(format!(
                "DUSTLINK_BACKEND points to missing file: {}",
                value.display()
            ));
        }
        if let Some(found) = find_in_path(&value) {
            return Ok(found);
        }
        return Err(format!(
            "DUSTLINK_BACKEND command not found in PATH: {}",
            value.display()
        ));
    }

    for candidate in ["ld.lld", "rust-lld"] {
        if let Some(found) = find_in_path(Path::new(candidate)) {
            return Ok(found);
        }
    }

    if let Some(found) = rust_lld_from_sysroot() {
        return Ok(found);
    }

    Err("no linker backend found; install ld.lld or rustup toolchain with rust-lld".to_string())
}

fn rust_lld_from_sysroot() -> Option<PathBuf> {
    let sysroot = rustc_cmd_output(&["--print", "sysroot"])?;
    let host = rustc_host_triple()?;
    let mut candidates = vec![
        PathBuf::from(&sysroot).join(format!("lib/rustlib/{}/bin/rust-lld", host)),
        PathBuf::from(&sysroot).join("lib/rustlib/bin/rust-lld"),
    ];
    if cfg!(windows) {
        candidates = candidates
            .into_iter()
            .flat_map(|p| vec![p.with_extension("exe"), p])
            .collect();
    }

    for path in candidates {
        if path.is_file() {
            return Some(path);
        }
    }

    let rustlib = PathBuf::from(&sysroot).join("lib").join("rustlib");
    if let Ok(entries) = std::fs::read_dir(rustlib) {
        for entry in entries.flatten() {
            let p = entry.path().join("bin").join(if cfg!(windows) {
                "rust-lld.exe"
            } else {
                "rust-lld"
            });
            if p.is_file() {
                return Some(p);
            }
        }
    }

    None
}

fn rustc_host_triple() -> Option<String> {
    let version = rustc_cmd_output(&["-vV"])?;
    for line in version.lines() {
        if let Some(rest) = line.strip_prefix("host: ") {
            return Some(rest.trim().to_string());
        }
    }
    None
}

fn rustc_cmd_output(args: &[&str]) -> Option<String> {
    let out = Command::new("rustc").args(args).output().ok()?;
    if !out.status.success() {
        return None;
    }
    let s = String::from_utf8_lossy(&out.stdout).trim().to_string();
    if s.is_empty() {
        None
    } else {
        Some(s)
    }
}

fn find_in_path(cmd: &Path) -> Option<PathBuf> {
    if cmd.components().count() > 1 || cmd.is_absolute() {
        return cmd.is_file().then(|| cmd.to_path_buf());
    }

    let path_env = env::var_os("PATH")?;
    let cmd_name = cmd.as_os_str();

    #[cfg(windows)]
    let extensions: Vec<OsString> = {
        let mut out = Vec::new();
        if let Some(pathext) = env::var_os("PATHEXT") {
            let as_string = pathext.to_string_lossy();
            for ext in as_string.split(';').filter(|x| !x.is_empty()) {
                out.push(OsString::from(ext.trim_start_matches('.')));
            }
        }
        if out.is_empty() {
            out = vec![
                OsString::from("EXE"),
                OsString::from("CMD"),
                OsString::from("BAT"),
            ];
        }
        out
    };

    for dir in env::split_paths(&path_env) {
        let base = dir.join(cmd_name);
        if base.is_file() {
            return Some(base);
        }

        #[cfg(windows)]
        {
            for ext in &extensions {
                let candidate = base.with_extension(ext);
                if candidate.is_file() {
                    return Some(candidate);
                }
            }
        }
    }

    None
}

fn is_flag(arg: &OsString, want: &str) -> bool {
    arg.to_string_lossy() == want
}

fn has_flavor_flag(args: &[OsString]) -> bool {
    args.iter().any(|a| {
        let s = a.to_string_lossy();
        s == "-flavor" || s == "--flavor" || s.starts_with("--flavor=")
    })
}

fn is_rust_lld(path: &Path) -> bool {
    let name = path
        .file_name()
        .map(|x| x.to_string_lossy().to_ascii_lowercase())
        .unwrap_or_default();
    name == "rust-lld" || name == "rust-lld.exe"
}
