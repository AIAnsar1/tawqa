use clap::{Parser, Subcommand};




#[derive(Subcommand, Debug)]
pub enum Command {
    #[clap(alias = "l")]
    Listen {
        #[clap(short, long, name = "interactive")]
        interactive: bool,
        #[clap(short, long, conflicts_with = "local-interactive")]
        block_signals: bool,
        #[clap(short,long,name = "local-interactive",conflicts_with = "interactive")]
        local_interactive: bool,
        #[clap(short, long)] // hidden
        exec: Option<String>,
        #[clap(num_args = ..=2)]
        host: Vec<String>,
    },
    #[clap(alias = "c")]
    Connect {
        #[clap(short, long)]
        shell: String,
        #[clap(num_args = ..=2)]
        host: Vec<String>,
    },
}


#[derive(Parser, Debug)]
#[clap(name = "tawqa", version, arg_required_else_help(true))]
pub struct Opts {
    #[clap(subcommand)]
    pub command: Command
}



























