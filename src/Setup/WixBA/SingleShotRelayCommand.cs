//-------------------------------------------------------------------------------------------------
// <copyright file="SingleShotRelayCommand.cs" company="Outercurve Foundation">
//   Copyright (c) 2004, Outercurve Foundation.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
//-------------------------------------------------------------------------------------------------

namespace Microsoft.Tools.WindowsInstallerXml.UX
{
    using System;

    public class SingleShotRelayCommand : RelayCommand
    {
        private bool executed = false;

        public SingleShotRelayCommand(Action<object> execute, Predicate<object> canExecute)
            : base(execute, canExecute)
        {
        }

        public override bool CanExecute(object parameter)
        {
 	        return !executed && base.CanExecute(parameter);
        }

        public override void Execute(object parameter)
        {
            this.executed = true;
            base.Execute(parameter);
        }
    }
}
