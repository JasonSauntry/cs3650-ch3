#!/usr/bin/perl
use 5.16.0;
use warnings FATAL => 'all';

# use Test::Simple tests => 42;
use IO::Handle;
use Data::Dumper;

sub write_text {
    my ($name, $data) = @_;
    open my $fh, ">", "mnt/$name" or return;
    $fh->say($data);
    close $fh;
}

write_text("foo", "Foobar");
