#!/usr/bin/perl

=pod
USAGE is coutAnalysis.pl runForinput.cout
OUTPUT is 2 columns
Module TimeSpent
=cut

use warnings;
use strict;
use utf8;

my ($file) = @ARGV;
defined($file) or die "USAGE: $0 file\n";

my %h;
my $totalTime = loadData(\%h, $file);

print STDERR "#ElementsObserved=".scalar(keys %h)."\n";
printData(\%h, $totalTime);
print STDERR "#TotalForRun=$totalTime\n";

sub loadData
{
	my ($h, $file) = @_;
	open(FILE, "<", $file) or die "$0: Cannot open $file : $!\n";

	my ($firstT, $lastT);

	my $observed = 0;
	while (<FILE>) {
		#LanczosSolver [7.714]: starting clock
		next unless (/^([^ ]+) \[([\d\.]+)\]\: ([^ ]+) clock/);
		my $name = $1;
		my $t = $2;
		my $what = $3;
		(($name) and ($t) and ($what)) or next;
		next if ($what ne "starting" and $what ne "stopping");

		$firstT = $t if (!defined($firstT));
		$lastT = $t;

		if (defined($h->{$name})) {
			my $ptr = $h->{$name};
			if (scalar(@$ptr) != 4) {
				print STDERR "$0: Error with $name\n";
				next;
			}

			my $value = $ptr->[1];
			my $dt = 0;
			my $otherness = $ptr->[3];
			if ($what eq "starting") {
				if ($value != 0) {
					print STDERR "$0: $name starting without stopping at line $.\n";
					next;
				}

				$value = $t;

			} elsif ($what eq "stopping") {
				if ($value > $t) {
					print STDERR "$0: $name has time skew\n";
					next;
				}

				$dt = $t - $value;
				# Increase otherness by dt for all labels that are in open mode
				otherNess($h, $dt);
				$value = 0;
				
				if ($dt < $otherness) {
					die "$0: Otherness $otherness greater than $dt for $name\n";
				}
	
				$dt -= $otherness;
				$otherness = 0;
				$observed += $dt;
			} else {
				print STDERR "$0: expecting starting or stopping not $what\n";
				next;
			}

			my @temp = ($ptr->[0] + $dt, $value, $ptr->[2] + 1, $otherness);
			$h->{$name} = \@temp;
			
		} else {
			if ($what ne "starting") {
				print STDERR "$0: expecting starting not $what\n";
				next;
			}

			my @temp = (0, $t, 1, 0);
			$h->{$name} = \@temp
		}
	}

	close(FILE);
	
	my $total = $lastT - $firstT;
	if ($total < $observed) {
		print STDERR "total $total is less than observed $observed\n";
		return $total;
	}

	my $unobserved = $total - $observed;
	my @temp = ($unobserved, 0, 0, 0);
	$h->{"Unobserved"} = \@temp;

	return $total;
}

sub otherNess
{
	my ($h, $dt) = @_;
	foreach my $k (keys %$h) {
		my $ptr = $h->{$k};
		next if ($ptr->[1] == 0);
		$ptr->[3] += $dt;
	}
}

sub printData
{
	my ($hptr, $totalTime) = @_;
	my %h = %$hptr;
	my @ls = (25, 14, 12);
	my $h1 = toFixedLength("Module", $ls[0], "center");
	my $h2 = toFixedLength("SelfInSeconds", $ls[1], "center", 3);
	my $h3 = toFixedLength("SelfPerCent", $ls[2], "center", 1);
	my $h4 = toFixedLength("TimesCalled", $ls[2], "center");
	my $sep = multiChar(" ", 4);
	my $sep2 = multiChar(" ", 4);
	print "$h1$sep$h2$sep2$h3$sep2$h4\n";
	my $c = {"time" => 0, "percent" => 0};

	foreach my $k (sort {$h{$b}->[0] <=> $h{$a}->[0]} keys %h) {
		my $ptr = $hptr->{$k};
		if (scalar(@$ptr) != 4) {
			print STDERR "$0: Error with $k\n";
			next;
		}

		my $name = toFixedLength($k, $ls[0], "after");
		my $t = int($ptr->[0]*1000)/1000;
		$c->{"time"} += $t;
		my $time = toFixedLength($t, $ls[1], "before", 3);
		my $shouldBeZero = $ptr->[1];
		if ($shouldBeZero != 0) {
			print STDERR "$0: Error with $k, expecting 0, got $shouldBeZero\n";
			next;
		}

		my $perCent = int($t*1000/$totalTime)/10;
		$c->{"percent"} += $perCent;
		my $perCentPrint = toFixedLength($perCent." %", $ls[2], "before", 1);
		my $numberOfTimes = toFixedLength($ptr->[2], $ls[2], "before");
		print "$name$sep$time$sep2$perCentPrint$sep2$numberOfTimes\n";
	}

	my ($t, $perCent) = ($c->{"time"}, $c->{"percent"});
	my $name = toFixedLength("Totals", $ls[0], "after");
	my $time = toFixedLength($t, $ls[1], "before", 3);
	my $perCentPrint = toFixedLength($perCent." %", $ls[2], "before", 1);
	print  "--------------------\n";
	print "$name$sep$time$sep2$perCentPrint\n";
}

sub toFixedLength
{
	my ($what, $n, $alignment, $prec) = @_;
	my $x = correctDecimalPointIfNeeded($what, $prec);
	my $l = length($x);
	return $x if ($l >= $n);
	my $spaces = multiChar(" ", $n - $l);
	if ($alignment eq "after") {
		return "$x$spaces";
	} elsif ($alignment eq "before") {
		return "$spaces$x";
	}

	my $nml = $n - $l;
	++$nml if ($nml & 1);
	my $h = multiChar(" ", int($nml/2));
	return "$h$x$h";
}

sub correctDecimalPointIfNeeded
{
	my ($x, $prec) = @_;
	if ($x =~ /(^\d+)\.(\d*$)/) {
		my $a = $1;
		my $b = $2;
		my $l = length($b);
		return "$x" if ($l >= $prec);
		my $paddingLength = $prec - $l;
		my $pad = multiChar("0", $paddingLength);
		return "$x"."$pad";
	}
	
	return "$x";	
}

sub multiChar
{
	my ($c, $n) = @_;
	my $sum = "";
	for (my $i = 0; $i < $n; ++$i) {
		$sum .= "$c";
	}

	return $sum;
}

