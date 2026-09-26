{
  description = "foxticulum";

  inputs = {
    nixpkgs.url = "nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    { }
    // flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
      in
      rec {
        devShells.default = pkgs.stdenv.mkDerivation {
          name = "foxticulum";
          buildInputs = with pkgs; [
            gnumake
            gcc
            (stdenv.mkDerivation {
              name = "tweetnacl";

              srcs = [
                (fetchurl {
                  url = "https://tweetnacl.cr.yp.to/20140427/tweetnacl.c";
                  hash = "sha256-AuZbwwE/8haJgzZeVZBrx4PEx+CmDYEA8XuzA6FxdcQ=";
                })
                (fetchurl {
                  url = "https://tweetnacl.cr.yp.to/20140427/tweetnacl.h";
                  hash = "sha256-Q/Ka1yHZknt0ewEAq0FgwRnnuxgMfJimbkv3nTEkQoc=";
                })
              ];

              unpackPhase = ''
                runHook preUnpack

                for _src in $srcs; do
                  cp "$_src" $(stripHash "$_src")
                done

                runHook postUnpack
              '';

              buildPhase = ''
                gcc -fpic -O2 -c tweetnacl.c -o tweetnacl.o
                gcc -shared -o libtweetnacl.so tweetnacl.o
              '';

              installPhase = ''
                mkdir -p $out/lib
                mkdir -p $out/include

                cp libtweetnacl.so $out/lib/
                cp tweetnacl.h $out/include/
              '';
            })
          ];
        };
      }
    );
}
