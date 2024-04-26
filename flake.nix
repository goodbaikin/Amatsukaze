{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.11";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};

        in
        {
          devShells.default = pkgs.stdenv.mkDerivation {
            name = "amatsukaze";
            src = self;
            buildInputs = with pkgs; [
              libgcc
              gdb
              openssl
              gnumake
              nasm
              cmake
              ninja
              docker
            ];
          };
        }
      );
}