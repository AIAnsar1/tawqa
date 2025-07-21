use std::fs::OpenOptions;
use std::io::Result;
use std::os::unix::io::AsRawFd;
use termios::{Termios, ECHO, ICANON, tcsetattr, TCSADRAIN};

pub fn setup_fd() -> Result<()> {
    let tty = OpenOptions::new().write(true).read(true).open("/dev/tty")?;
    let fd = tty.as_raw_fd();
    let mut termios = Termios::from_fd(fd)?;
    termios.c_lflag &= !(ECHO | ICANON);
    tcsetattr(fd, TCSADRAIN, &termios)?;
    Ok(())
}