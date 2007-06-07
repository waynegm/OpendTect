#_Pmake________________________________________________________________________
# Puts the header file dependencies for a module into a file '.deps'
# Author: Bert, May 2007
#
# $Id: gendeps.pl,v 1.5 2007-06-07 19:24:44 cvsbert Exp $
#______________________________________________________________________________


do go();


sub go
{
    do initVars();

    open( OUTPUTFILE, "> $outfile" );
    print OUTPUTFILE "# ************************************\n";
    print OUTPUTFILE "# *** File generated by gendeps.pl ***\n";
    print OUTPUTFILE "# *** Source file dependencies     ***\n";
    print OUTPUTFILE "# ************************************\n";

    my $fil;
    print "Getting deps for:" if ( $dbglvl eq 1 );
    foreach $fil ( @srcfiles )
    {
	print " $fil" if ( $dbglvl eq 1 );
	do genDeps( $fil );
	do cleanDeps();
	do outputDeps( $fil );
    }
    print " done.\n" if ( $dbglvl eq 1 );

    close OUTPUTFILE;
}


sub doExit
{
    my $msg = shift(@_); my $rv = shift(@_);
    if ( $msg != "" && ($dbglvl || $rv) )
	{ warn $msg }
    exit $rv;
}


sub initVars
{
    do setDefaults();
    do parseCmdLine();
    do getNoDeps();

    if ( $dbglvl eq 2 )
    {
	print "\ndbglvl: $dbglvl\n"; print "cpp: $cpp\n";
	print "outfile: $outfile\n"; print "srcfiles: @srcfiles\n";
	print "idirectives: @idirectives\n"; print "ipathnames: @ipathnames\n";
    }
}


sub genDeps
{
    my $fil = shift;
    $fil =~ s/^=//; $fil =~ s/^\+//;

    my @gendepcmd = (@gendepcmdstart, $fil, "|");
    print "gendepcmd: @gendepcmd\n" if ( $dbglvl eq 2 );

    open( CMD, "@gendepcmd" ) || doExit( "Failed to run @gendepcmd", 1 );

    @deps = ();
    while ( <CMD> )
    {
        foreach $fil ( split )
	{
	    if ( !($fil =~ "\.o:" || $fil eq '\\') )
		{ @deps = ( @deps, $fil ); }
	}
    }

    $fil = getOForCc( $fil );
    print "Deps for $fil: @deps\n" if ( $dbglvl == 2 );
    close CMD;
}


sub cleanDeps
{
    my @tmplist = ();
    my $fil;
    foreach $fil ( @deps )
    {
    	my $addfil = 1; my $nodepdir;
	foreach $nodepdir ( @nodepdirs )
	    { if ( $fil =~ m/$nodepdir/ ) { $addfil = 0; last; } }
	if ( $addfil ) { @tmplist = (@tmplist, $fil); }
    }

    @deps = ();
    @alreadyadded = ();
    foreach $fil ( @tmplist )
    {
	next if ( !($fil =~ /\.h/) ); # remove any thing that's not .h

    	my $path;
	foreach $path ( @ipathnames ) # remove known -I paths
	    { $fil =~ s%$path/%%; }

	next if ( $fil =~ /[^\/].*\// ); # remove any relative path left

	@deps = (@deps, $fil);
    }

    # Now remove duplicates
    undef %fil;
    @deps = grep( !$fil{$_}++, @deps );
}


sub outputDeps
{
    my $fil = shift;
    my $isprog = 0;
    if ( $fil =~ /^\+/ ) { $isprog = 1; }
    $fil =~ s/^=//; $fil =~ s/^\+//;
    $fil = getOForCc( $fil );

    $start = "\n";
    if ( $isprog )
	{ $fil =~ s/\.o//; $start .= '$(BINDIR)/'."$fil: "; }
    else
	{ $start .= '$(LIBWORK)('."$fil): "; }
    my $depsstr = join ' ', @deps;
    print OUTPUTFILE $start, $depsstr, "\n";
}


sub setDefaults
{
    $dbglvl = 0;
    $cpp = "$ENV{'GNUCC'}";
    $cpp = "gcc" if ( "$cpp" eq "" );
    $cppflags = "-D$ENV{'HDIR'} -MM";
    $outfile = ".deps";
}


sub parseCmdLine
{
    while ( @ARGV && ($_ = $ARGV[0]) =~ /^--(.*)/ )
    {
	if ( $_ eq "--v" )
	    { $dbglvl = 1; }
	elsif ( $_ eq "--d" )
	    { $dbglvl = 2; }
	elsif ( $_ eq "--c" )
	    { $cpp = $ARGV[1]; shift( @ARGV ); }
	elsif ( $_ eq "--o" )
	    { $outfile = $ARGV[1]; shift( @ARGV ); }
	shift( @ARGV );

    }

    while ( @ARGV && ($_ = $ARGV[0]) =~ /^-(.)(.*)/ )
    {
	if ( $1 eq "I" )
	    { @idirectives = (@idirectives, $ARGV[0]); }
	shift( @ARGV );
    }

    while ( @ARGV && ($_ = $ARGV[0]) =~ /.*c[c]?/ )
    {
	@srcfiles = (@srcfiles, $_);
	shift( @ARGV );
    }

    @ipathnames = @idirectives;
    foreach ( @ipathnames )
	{ s/-I//g; s/(\.)/\\$1/g; }

    @gendepcmdstart = ($cpp, $cppflags, @idirectives);
}


sub getNoDeps
{
    my $nodepfile = "$ENV{'WORK'}/Pmake/NoDepend";
    if ( ! -e $nodepfile )
    {
	$nodepfile = "$ENV{'PMAKE'}/NoDepend";
	return if ( ! -e $nodepfile );
    }

    if ( !open(NODEP,"$nodepfile") )
    {
	warn "Cannot open NoDepend file";
	return;
    }

    while ( <NODEP> )
    {
	next line if /^#/; next line if /^\s*$/; chomp;
	s/(\W)/\\$1/g;
	@nodepdirs = (@nodepdirs, $_);
    }
    close NODEP;
}


sub printInitialVars
{
}


sub getOForCc
{
    my $fil = shift;
    $fil =~ s/\.c[c]?/.o/;
    return $fil;
}
