Name:           {{{git_dir_name}}}
Version:        {{{package_version}}}{{{git_version_appendix}}}
Release:        1%{?dist}
Summary:        A post-processor for faust, which allows to generate with more flexibility

License:        GPLv2+
URL:            https://github.com/jpcima/faustpp
VCS:            {{{git_dir_vcs}}}
Source0:        {{{git_dir_pack}}}
BuildArch:      noarch

BuildRequires:  python3-devel
BuildRequires:  python3dist(setuptools)

%description
A post-processor for faust, which allows to generate with more flexibility

This is a source transformation tool based on the Faust compiler.

It permits to arrange the way how faust source is generated with greater
flexibility.

Using a template language known as Jinja2, it is allowed to manipulate metadata
with iteration and conditional constructs, to easily generate custom code
tailored for the job. Custom metadata can be handled by the template mechanism.

%prep
{{{git_dir_setup_macro}}}

%build
%py3_build

%install
%py3_install

%files
%doc README.md
%license LICENSE LICENSE-EXCEPTION.md
%{_bindir}/%{name}
%{python3_sitelib}/%{name}/*
%{python3_sitelib}/%{name}-{{{package_version}}}-py%{python3_version}.egg-info

%changelog
{{{git_dir_changelog}}}
